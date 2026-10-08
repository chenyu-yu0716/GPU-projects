#pragma once

#include <cuda_runtime.h>

#include <complex.hpp>
#include <render_paramters.h>

void renderMandelbrotSet(float4* output, RenderParameters const& parameters
                         // add additional arguments if necessary
);

inline int computeMaxIterations(long double zoom) {
    long double const iterations = 256.0L + 128.0L * std::log2(std::max(zoom, 1.0L));
    return static_cast<int>(std::min(iterations, 20000.0L));
}