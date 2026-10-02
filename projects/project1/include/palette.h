#pragma once

#include <array>
#include <cuda_runtime.h>

struct Palette {
public:
    float period;
    float shift;
    float frequency;

    static int constexpr size = 256;

public:
    Palette();

    Palette(std::array<float3, Palette::size> const& colors, float period, float shift, float frequency);
    
    ~Palette();

    void upload(std::array<float3, Palette::size> const& colors);
};