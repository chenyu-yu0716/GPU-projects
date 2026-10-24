#include <scene.h>

#include <optix/utility.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Scene::Scene(optix::DeviceContext const& context, std::filesystem::path const& assetRootDir)
    : m_context(context) {
    m_modelMaterials = {
        Material{float3{1, 1, 1}, 0, 1.5f, Dielectric},
        Material{float3{.7f, .6f, .5f}, 0, 1.0f, Metal},
        Material{float3{.8f, .4f, .2f}, 0, 1.0f, Lambertian}
    };

    createGeometry(assetRootDir);
    createSkybox(assetRootDir);
}

void Scene::createGeometry(std::filesystem::path const& assetRootDir) {
    // Load and flatten Lucy's triangle mesh.
    auto const path = assetRootDir / "models/lucy.obj";

    tinyobj::ObjReader reader;
    tinyobj::ObjReaderConfig config{};
    if (!reader.ParseFromFile(path.string(), config)) {
        throw std::runtime_error("Unable to load Lucy: " + path.string() + " " + reader.Error());
    }

    auto const& attrib = reader.GetAttrib();
    std::vector<float3> vertices;
    std::vector<float3> normals;
    for (auto const& shape : reader.GetShapes()) {
        size_t offset = 0;
        for (auto count : shape.mesh.num_face_vertices) {
            if (count != 3) {
                throw std::runtime_error("Lucy has a non-triangle face");
            }

            float3 p[3], n[3];
            bool hasNormals = true;
            for (int j = 0; j < 3; ++j) {
                auto const idx = shape.mesh.indices.at(offset + j);
                if (idx.vertex_index < 0) {
                    throw std::runtime_error("Invalid Lucy vertex index");
                }
                p[j] = float3{
                    attrib.vertices.at(3 * idx.vertex_index),
                    attrib.vertices.at(3 * idx.vertex_index + 1),
                    attrib.vertices.at(3 * idx.vertex_index + 2)
                };
                if (idx.normal_index >= 0) {
                    n[j] = float3{
                        attrib.normals.at(3 * idx.normal_index),
                        attrib.normals.at(3 * idx.normal_index + 1),
                        attrib.normals.at(3 * idx.normal_index + 2)
                    };
                }
                else {
                    hasNormals = false;
                }
            }

            if (!hasNormals) {
                float3 const edge1{p[1].x - p[0].x, p[1].y - p[0].y, p[1].z - p[0].z};
                float3 const edge2{p[2].x - p[0].x, p[2].y - p[0].y, p[2].z - p[0].z};
                float3 normal{
                    edge1.y * edge2.z - edge1.z * edge2.y,
                    edge1.z * edge2.x - edge1.x * edge2.z,
                    edge1.x * edge2.y - edge1.y * edge2.x
                };
                float const inverseLength =
                    1.0f / std::sqrt(normal.x * normal.x + normal.y * normal.y + normal.z * normal.z);
                normal.x *= inverseLength;
                normal.y *= inverseLength;
                normal.z *= inverseLength;
                n[0] = n[1] = n[2] = normal;
            }

            for (int j = 0; j < 3; ++j) {
                vertices.push_back(p[j]);
                normals.push_back(n[j]);
            }
            offset += 3;
        }
    }

    if (vertices.empty()) {
        throw std::runtime_error("Lucy has no triangles");
    }

    // Upload the shared model mesh and build its GAS.
    m_vertices.reallocate(vertices.size());
    m_vertices.copyFromHost(vertices.data());
    m_normals.reallocate(normals.size());
    m_normals.copyFromHost(normals.data());

    CUdeviceptr vertexAddress = optix::address(m_vertices.data());
    uint32_t flag = OPTIX_GEOMETRY_FLAG_NONE;
    OptixBuildInput triangle{
        .type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES,
        .triangleArray = {
            .vertexBuffers = &vertexAddress,
            .numVertices = static_cast<uint32_t>(vertices.size()),
            .vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3,
            .flags = &flag,
            .numSbtRecords = 1,
        },
    };
    OptixAccelBuildOptions options{
        .buildFlags = OPTIX_BUILD_FLAG_PREFER_FAST_TRACE,
    };
    m_modelGas = optix::GeometryAccelerationStructure(m_context, options, {&triangle, 1});

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> random(0.0f, 1.0f);
    auto range = [&](float a, float b) { return a + (b - a) * random(rng); };
    auto rgb = [&](float a, float b) { return float3{range(a, b), range(a, b), range(a, b)}; };
    auto distance = [](float3 a, float3 b) {
        float const dx = a.x - b.x;
        float const dy = a.y - b.y;
        float const dz = a.z - b.z;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    };

    // Generate the ground, sphere field, and featured spheres.
    std::vector<float3> centers{float3{0, -2000, 0}};
    std::vector<float> radii{2000};
    std::vector<Material> materials{Material{float3{.5f, .5f, .5f}, 0, 1.0f, Lambertian}};
    for (int i = 0; i <= 100; i += 3) {
        for (int j = 0; j <= 60; j += 2) {
            float rho = 15.0f * i / 100.0f;
            float theta = 2.0f * std::numbers::pi_v<float> * j / 60.0f + .2f * random(rng);
            float y = range(.08f, .25f);
            float3 center{rho * std::cos(theta), y, rho * std::sin(theta)};
            if (distance(center, float3{0, .2f, 0}) <= 2 || distance(center, float3{4, .2f, -2}) <= 2 ||
                distance(center, float3{-4, .2f, 2}) <= 2 || distance(center, float3{4, 0, 5}) <= 1) {
                continue;
            }

            float choose = random(rng);
            Material material;
            if (choose < .4f) {
                auto a = rgb(0, 1), b = rgb(0, 1);
                material = Material{float3{a.x * b.x, a.y * b.y, a.z * b.z}, 0, 1.0f, Lambertian};
            }
            else if (choose < .95f) {
                material = Material{rgb(.5f, 1), range(0, .5f), 1.0f, Metal};
            }
            else {
                material = Material{float3{1, 1, 1}, 0, 1.5f, Dielectric};
            }
            centers.push_back(center);
            radii.push_back(y);
            materials.push_back(material);
        }
    }

    centers.push_back(float3{3.7f, 1.2f, 4.7f});
    radii.push_back(1.2f);
    materials.push_back(Material{float3{1, 1, 1}, 0, 1.5f, Dielectric});
    centers.push_back(float3{-8, 1, -2});
    radii.push_back(1);
    materials.push_back(Material{float3{.2f, .4f, .8f}, 0, 1.0f, Lambertian});
    centers.push_back(float3{3, 3, -8});
    radii.push_back(2.4f);
    materials.push_back(Material{float3{.7f, .6f, .5f}, 0, 1.0f, Metal});

    // Upload sphere data and build its GAS.
    m_sphereCenters.reallocate(centers.size());
    m_sphereCenters.copyFromHost(centers.data());
    m_sphereRadii.reallocate(radii.size());
    m_sphereRadii.copyFromHost(radii.data());
    m_sphereMaterials.reallocate(materials.size());
    m_sphereMaterials.copyFromHost(materials.data());

    CUdeviceptr centerAddress = optix::address(m_sphereCenters.data());
    CUdeviceptr radiusAddress = optix::address(m_sphereRadii.data());
    OptixBuildInput spheres{
        .type = OPTIX_BUILD_INPUT_TYPE_SPHERES,
        .sphereArray = {
            .vertexBuffers = &centerAddress,
            .numVertices = static_cast<uint32_t>(centers.size()),
            .radiusBuffers = &radiusAddress,
            .flags = &flag,
            .numSbtRecords = 1,
        },
    };
    m_sphereGas = optix::GeometryAccelerationStructure(m_context, options, {&spheres, 1});

    // Instance the shared model and assemble the top-level IAS.
    constexpr std::array<float, 12> identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    std::array<OptixInstance, 4> instances{};
    constexpr std::array<std::array<float, 2>, 3> positions{{{0, 0}, {-4, 2}, {4, -2}}};

    for (int i = 0; i < 3; ++i) {
        auto [tx, tz] = positions[i];
        OptixStaticTransform node{};
        node.child = m_modelGas.handle();
        float forward[12] = {0, 0, .6f, tx, 0, .6f, 0, 0, -.6f, 0, 0, tz};
        float inverse[12] = {0, 0, -1 / .6f, tz / .6f, 0, 1 / .6f, 0, 0, 1 / .6f, 0, 0, -tx / .6f};
        std::copy_n(forward, 12, node.transform);
        std::copy_n(inverse, 12, node.invTransform);
        m_modelNodes[i] = optix::StaticTransform(m_context, node);
        instances[i] = optix::makeInstance(m_modelNodes[i].handle(), identity, i, i);
    }

    instances[3] = optix::makeInstance(m_sphereGas.handle(), identity, 3, 3);
    m_acceleration = optix::InstanceAccelerationStructure(m_context, options, instances);
}

void Scene::createSkybox(std::filesystem::path const& assetRootDir) {
    constexpr std::array<char const*, 6> names{
        "right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"
    };

    for (size_t face = 0; face < names.size(); ++face) {
        auto path = assetRootDir / "skyboxrt" / names[face];

        int w = 0, h = 0, channels = 0;
        unsigned char* pixels = stbi_load(path.string().c_str(), &w, &h, &channels, 4);
        if (!pixels) {
            throw std::runtime_error("Unable to load skybox: " + path.string());
        }

        if (!m_skybox) {
            if (w <= 0 || w != h) {
                stbi_image_free(pixels);
                throw std::runtime_error("Skybox faces must be square");
            }

            gpu::TextureCubemap::Specification spec{
                .size = static_cast<size_t>(w),
                .format = cudaCreateChannelDesc<uchar4>(),
                .texDesc = {
                    .addressMode = {cudaAddressModeClamp, cudaAddressModeClamp, cudaAddressModeClamp},
                    .filterMode = cudaFilterModeLinear,
                    .readMode = cudaReadModeNormalizedFloat,
                    .normalizedCoords = 1,
                },
            };

            m_skybox = std::make_unique<gpu::TextureCubemap>(spec);
        }

        if (w != m_skybox->size() || h != m_skybox->size()) {
            stbi_image_free(pixels);
            throw std::runtime_error("Skybox sizes differ");
        }
        m_skybox->copyFromHost(static_cast<gpu::TextureCubemap::Face>(face), pixels, static_cast<size_t>(w) * 4);
        stbi_image_free(pixels);
    }
}
