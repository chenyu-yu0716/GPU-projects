#include <array>
#include <fstream>
#include <gpu/utility.h>
#include <iterator>
#include <optix/module.h>
#include <optix/program_group.h>
#include <optix_renderer.h>
#include <optix_stack_size.h>
#include <span>
#include <stdexcept>
#include <string>

OptixRenderer::OptixRenderer(optix::DeviceContext const& context,
                             Scene const& scene,
                             std::filesystem::path const& modulePath,
                             uint32_t width,
                             uint32_t height)
    : m_context(context)
    , m_scene(scene)
    , m_denoiser(
          context, OPTIX_DENOISER_MODEL_KIND_TEMPORAL_AOV, OptixDenoiserOptions{.guideAlbedo = 1, .guideNormal = 1}) {
    createPipelineAndShaderBindingTable(modulePath);
    m_launchParams.reallocate(1);
    resize(width, height);
}

void OptixRenderer::createPipelineAndShaderBindingTable(std::filesystem::path const& modulePath) {
    std::ifstream file(modulePath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open OptiX module: " + modulePath.string());
    }
    std::string moduleData(std::istreambuf_iterator<char>{file}, {});

    // Create the pipeline.
    OptixPipelineCompileOptions pipelineOptions{
        .traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY,
        .numPayloadValues = 2,   // Two 32-bit values carry the Hit pointer through optixTrace.
        .numAttributeValues = 2, // Two triangle barycentric coordinates are used
                                 // as intersection attributes.
        .pipelineLaunchParamsVariableName = "params",
        .usesPrimitiveTypeFlags =
            static_cast<unsigned int>(OPTIX_PRIMITIVE_TYPE_FLAGS_TRIANGLE | OPTIX_PRIMITIVE_TYPE_FLAGS_SPHERE),
    };

    OptixModuleCompileOptions moduleOptions{
        .optLevel = OPTIX_COMPILE_OPTIMIZATION_DEFAULT,
        .debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_NONE,
    };

    optix::Module module(m_context, moduleOptions, pipelineOptions, moduleData);

    OptixBuiltinISOptions sphereOptions{
        .builtinISModuleType = OPTIX_PRIMITIVE_TYPE_SPHERE,
        .buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE,
    };

    OptixModule sphereModuleHandle{};
    CHECK_OPTIX(optixBuiltinISModuleGet(
        m_context.handle(), &moduleOptions, &pipelineOptions, &sphereOptions, &sphereModuleHandle));

    optix::Module sphereModule = optix::Module::adopt(sphereModuleHandle);

    std::array<OptixProgramGroupDesc, 4> desc{};

    desc[0].kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    desc[0].raygen.module = module.handle();
    desc[0].raygen.entryFunctionName = "__raygen__render";

    desc[1].kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    desc[1].miss.module = module.handle();
    desc[1].miss.entryFunctionName = "__miss__background";

    desc[2].kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    desc[2].hitgroup.moduleCH = module.handle();
    desc[2].hitgroup.entryFunctionNameCH = "__closesthit__triangle";

    desc[3].kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    desc[3].hitgroup.moduleCH = module.handle();
    desc[3].hitgroup.entryFunctionNameCH = "__closesthit__sphere";
    desc[3].hitgroup.moduleIS = sphereModule.handle();

    OptixProgramGroupOptions groupOptions{};
    std::array<optix::ProgramGroup, 4> programGroups{};
    for (size_t i = 0; i < programGroups.size(); ++i) {
        programGroups[i] = optix::ProgramGroup(m_context, desc[i], groupOptions);
    }

    OptixPipelineLinkOptions linkOptions{.maxTraceDepth = 1};
    m_pipeline = optix::Pipeline(m_context, pipelineOptions, linkOptions, programGroups);

    OptixStackSizes stackSizes{};
    for (optix::ProgramGroup const& group : programGroups) {
        CHECK_OPTIX(optixUtilAccumulateStackSizes(group.handle(), &stackSizes, m_pipeline.handle()));
    }
    uint32_t traversal = 0, state = 0, continuation = 0;
    CHECK_OPTIX(optixUtilComputeStackSizes(&stackSizes, 1, 0, 0, &traversal, &state, &continuation));
    m_pipeline.setStackSize(traversal, state, continuation, 3);

    // Create the shader binding table.
    optix::SbtRecord<> raygen{}, miss{};
    optix::packHeader(programGroups[0].handle(), raygen);
    optix::packHeader(programGroups[1].handle(), miss);

    std::array<optix::SbtRecord<HitRecordData>, 4> hits{};
    auto const& modelMaterials = m_scene.modelMaterials();
    for (int i = 0; i < 3; ++i) {
        optix::packHeader(programGroups[2].handle(), hits[i]);
        hits[i].data.normals = m_scene.triangleNormals();
        hits[i].data.material = modelMaterials[i];
    }

    optix::packHeader(programGroups[3].handle(), hits[3]);
    hits[3].data.sphereMaterials = m_scene.sphereMaterials();

    m_sbt.setRaygen(&raygen, sizeof(raygen));
    m_sbt.setMiss(&miss, sizeof(miss), 1);
    m_sbt.setHitgroup(hits.data(), sizeof(hits[0]), static_cast<uint32_t>(hits.size()));
}

void OptixRenderer::resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || (width == m_width && height == m_height)) {
        return;
    }

    size_t const pixelCount = static_cast<size_t>(width) * height;
    m_accumulation.reallocate(pixelCount);
    m_albedo.reallocate(pixelCount);
    m_normal.reallocate(pixelCount);

    m_surfacePositions.reallocate(pixelCount);
    m_previousSurfacePositions.reallocate(pixelCount);
    m_flow.reallocate(pixelCount);
    m_flowTrustworthiness.reallocate(pixelCount);
    m_previousOutput.reallocate(pixelCount);

    m_denoiser.setup(width, height);
    size_t const internalGuideLayerBytes = pixelCount * m_denoiser.sizes().internalGuideLayerPixelSizeInBytes;
    m_previousInternalGuideLayer.reallocate(internalGuideLayerBytes);
    m_internalGuideLayer.reallocate(internalGuideLayerBytes);

    m_width = width;
    m_height = height;
    resetTemporalHistory();
}

// Describe a dense device image in the layout expected by the denoiser.
template <typename T>
static OptixImage2D denoiserImage(
    T const* data, uint32_t width, uint32_t height, OptixPixelFormat format, size_t pixelStrideInBytes = sizeof(T)) {
    return OptixImage2D{
        .data = optix::address(data),
        .width = width,
        .height = height,
        .rowStrideInBytes = static_cast<unsigned int>(width * pixelStrideInBytes),
        .pixelStrideInBytes = static_cast<unsigned int>(pixelStrideInBytes),
        .format = format,
    };
}

void OptixRenderer::denoiseAccumulation(float4* pixels) {
    OptixImage2D const color = denoiserImage(m_accumulation.data(), m_width, m_height, OPTIX_PIXEL_FORMAT_FLOAT4);

    // The raw average stays in m_accumulation; only the output pixels receive the denoised image.
    // Without history, the first frame of a sequence uses its own input as the previous output.
    OptixDenoiserLayer const layer{
        .input = color,
        .previousOutput = m_hasTemporalHistory
                              ? denoiserImage(m_previousOutput.data(), m_width, m_height, OPTIX_PIXEL_FORMAT_FLOAT4)
                              : color,
        .output = denoiserImage(pixels, m_width, m_height, OPTIX_PIXEL_FORMAT_FLOAT4),
        .type = OPTIX_DENOISER_AOV_TYPE_BEAUTY,
    };

    size_t const internalGuidePixelBytes = m_denoiser.sizes().internalGuideLayerPixelSizeInBytes;
    OptixDenoiserGuideLayer const guide{
        .albedo = denoiserImage(m_albedo.data(), m_width, m_height, OPTIX_PIXEL_FORMAT_FLOAT4),
        .normal = denoiserImage(m_normal.data(), m_width, m_height, OPTIX_PIXEL_FORMAT_FLOAT3),
        .flow = denoiserImage(m_flow.data(), m_width, m_height, OPTIX_PIXEL_FORMAT_FLOAT2),
        .previousOutputInternalGuideLayer = denoiserImage(m_previousInternalGuideLayer.data(),
                                                          m_width,
                                                          m_height,
                                                          OPTIX_PIXEL_FORMAT_INTERNAL_GUIDE_LAYER,
                                                          internalGuidePixelBytes),
        .outputInternalGuideLayer = denoiserImage(m_internalGuideLayer.data(),
                                                  m_width,
                                                  m_height,
                                                  OPTIX_PIXEL_FORMAT_INTERNAL_GUIDE_LAYER,
                                                  internalGuidePixelBytes),
        .flowTrustworthiness =
            denoiserImage(m_flowTrustworthiness.data(), m_width, m_height, OPTIX_PIXEL_FORMAT_FLOAT1),
    };

    OptixDenoiserParams const params{.temporalModeUsePreviousLayers = m_hasTemporalHistory ? 1u : 0u};
    m_denoiser.invoke(params, guide, std::span(&layer, 1));

    // Keep this frame's output and internal guide as history for the next frame.
    CHECK_CUDA(cudaMemcpy(m_previousOutput.data(), pixels, m_previousOutput.sizeBytes(), cudaMemcpyDeviceToDevice));
    m_previousInternalGuideLayer.swap(m_internalGuideLayer);
    m_hasTemporalHistory = true;
}

void OptixRenderer::resetTemporalHistory() {
    // The next denoised frame starts a new sequence; stale buffers are then ignored.
    m_hasTemporalHistory = false;
}

void OptixRenderer::render(
    float4* pixels, uint32_t firstSampleIndex, Camera const& camera, bool denoise, uint32_t samplesPerFrame) {
    // Motion data is only written while denoising, so a raw frame breaks the sequence.
    if (!denoise) {
        resetTemporalHistory();
    }

    glm::vec3 const position = camera.position();
    glm::vec3 const front = camera.getFront();
    glm::vec3 const right = camera.getRight();
    glm::vec3 const up = camera.getUp();
    LaunchParams launchParams{
        .pixels = pixels,
        .accumulation = m_accumulation.data(),
        .albedo = m_albedo.data(),
        .normal = m_normal.data(),
        .surfacePositions = m_surfacePositions.data(),
        .previousSurfacePositions = m_previousSurfacePositions.data(),
        .flow = m_flow.data(),
        .flowTrustworthiness = m_flowTrustworthiness.data(),
        .scene = m_scene.traversable(),
        .sky = m_scene.skyboxTexture(),
        .cameraPosition{position.x, position.y, position.z},
        .front{front.x, front.y, front.z},
        .right{right.x, right.y, right.z},
        .up{up.x, up.y, up.z},
        .previousCameraPosition = m_previousCameraPosition,
        .previousFront = m_previousFront,
        .previousRight = m_previousRight,
        .previousUp = m_previousUp,
        .width = m_width,
        .height = m_height,
        .writeGuides = denoise ? 1u : 0u,
        .computeFlow = denoise && m_hasTemporalHistory ? 1u : 0u,
    };

    for (uint32_t sampleOffset = 0; sampleOffset < samplesPerFrame; ++sampleOffset) {
        launchParams.sample = firstSampleIndex + sampleOffset;
        launchParams.frameIndex = m_frameIndex++;
        m_launchParams.copyFromHost(&launchParams);
        m_pipeline.launch(
            m_sbt, optix::address(m_launchParams.data()), sizeof(LaunchParams), nullptr, m_width, m_height);
    }

    if (denoise) {
        denoiseAccumulation(pixels);

        // This frame's surfaces and camera become the reference for the next frame's flow.
        m_previousSurfacePositions.swap(m_surfacePositions);
        m_previousCameraPosition = launchParams.cameraPosition;
        m_previousFront = launchParams.front;
        m_previousRight = launchParams.right;
        m_previousUp = launchParams.up;
    }
}
