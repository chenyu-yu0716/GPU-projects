#pragma once

#include <high_precision_float.h>
#include <complex.hpp>

#include <filesystem>

// Decimal strings intentionally preserve user-provided deep-zoom coordinates
// until the FP64 CUDA parameters are derived from their high-precision values.
struct OfflineRenderConfig {
    HighPrecisionComplex center;
    HighPrecisionFloat zoom;
    int width{0};
    int height{0};
    bool antialias{false};
    std::filesystem::path outputPath;
};

// Renders one image without creating an OpenGL context and writes an RGBA PNG.
// Throws std::invalid_argument for invalid options and std::runtime_error for
// CUDA or image-writing failures.
void renderMandelbrotSetOffline(OfflineRenderConfig const& config);
