#pragma once

#include <render_paramters.hpp>
#include <cuda_runtime.h>

void uploadVideoPalette(const float* rgb, std::size_t floatCount);

template <typename Real>
void renderMandelbrotSet(
    float4* __restrict__ output,
    Complex<Real> const* __restrict__ referenceOrbit,
    Complex<Real> const* __restrict__ seriesCoeff,
    const float* __restrict__ palette,
    const size_t paletteSize,
    RenderParameters<Real> const& params);
