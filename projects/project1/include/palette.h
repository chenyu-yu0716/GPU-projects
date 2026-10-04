#pragma once

#include <array>
#include <cuda_runtime.h>

struct Palette {
public:
    float period{1022.395737721f};
    float shift{0.0f};
    float frequency{1.0f};

    static int constexpr size = 256;

public:
    Palette();

    Palette(std::array<float3, Palette::size> const& colors, float period, float shift, float frequency);

    ~Palette() = default;

    void upload(std::array<float3, Palette::size> const& colors);

#if defined(__CUDACC__)
    __device__ float3 sample(float smoothIteration) const;

    __device__ static float3 sampleNormalized(float normalizedCoordinate);
#endif
};
