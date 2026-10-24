#pragma once
#include <cuda_runtime.h>
#include <optix.h>
#include <cstdint>

enum MaterialType : uint32_t { Lambertian, Metal, Dielectric };

struct Material {
    float3 albedo;
    float fuzz;
    float ior;
    uint32_t type;
};

struct HitRecordData {
    float3 const* normals;
    Material const* sphereMaterials;
    Material material;
};

struct LaunchParams {
    float4* pixels;
    float4* accumulation;
    float4* albedo;
    float3* normal;
    float4* surfacePositions; // xyz: world position; w: surface ID (zero for sky)
    float4 const* previousSurfacePositions;
    float2* flow;
    float* flowTrustworthiness;
    OptixTraversableHandle scene;
    cudaTextureObject_t sky;
    float3 cameraPosition;
    float3 front;
    float3 right;
    float3 up;
    float3 previousCameraPosition;
    float3 previousFront;
    float3 previousRight;
    float3 previousUp;
    uint32_t width;
    uint32_t height;
    uint32_t sample;
    uint32_t frameIndex;
    uint32_t writeGuides;
    uint32_t computeFlow;
};
