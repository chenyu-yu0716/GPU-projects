#pragma once

#include <camera.h>
#include <launch_params.h>
#include <scene.h>
#include <gpu/buffer.h>
#include <optix/denoiser.h>
#include <optix/device_context.h>
#include <optix/pipeline.h>
#include <optix/shader_binding_table.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>

class OptixRenderer {
public:
    OptixRenderer(
        optix::DeviceContext const& context,
        Scene const& scene,
        std::filesystem::path const& modulePath,
        uint32_t width,
        uint32_t height);

    void resize(uint32_t width, uint32_t height);

    void render(
        float4* pixels,
        uint32_t firstSampleIndex,
        Camera const& camera,
        bool denoise = true,
        uint32_t samplesPerFrame = 1);

private:
    // Borrowed dependencies; both must outlive the renderer.
    optix::DeviceContext const& m_context;
    Scene const& m_scene;

    // Ray tracing pipeline and shader records.
    optix::Pipeline m_pipeline;
    optix::ShaderBindingTable m_sbt;

    // OptiX denoiser and its persistent state.
    optix::Denoiser m_denoiser;

    // Image dimensions and RNG sequence, independent of accumulation resets.
    uint32_t m_width{};
    uint32_t m_height{};
    uint32_t m_frameIndex{};

    // Launch data and progressively averaged radiance.
    gpu::Buffer<LaunchParams> m_launchParams;
    gpu::Buffer<float4> m_accumulation;

    // Denoiser guides, progressively averaged like the radiance.
    gpu::Buffer<float4> m_albedo;
    gpu::Buffer<float3> m_normal;

    // Temporal denoiser motion data and history, swapped after every denoised frame.
    gpu::Buffer<float4> m_surfacePositions;
    gpu::Buffer<float4> m_previousSurfacePositions;
    gpu::Buffer<float2> m_flow;
    gpu::Buffer<float> m_flowTrustworthiness;
    gpu::Buffer<float4> m_previousOutput;
    gpu::Buffer<std::byte> m_previousInternalGuideLayer;
    gpu::Buffer<std::byte> m_internalGuideLayer;
    bool m_hasTemporalHistory{};

    // Camera basis of the previous denoised frame, used to compute flow.
    float3 m_previousCameraPosition{};
    float3 m_previousFront{};
    float3 m_previousRight{};
    float3 m_previousUp{};

private:
    void createPipelineAndShaderBindingTable(std::filesystem::path const& modulePath);

    void denoiseAccumulation(float4* pixels);

    void resetTemporalHistory();
};
