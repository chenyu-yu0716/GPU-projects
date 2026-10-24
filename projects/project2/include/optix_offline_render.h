#pragma once

#include <cstdint>
#include <filesystem>

struct OptixOfflineConfig {
    uint32_t width;
    uint32_t height;
    uint32_t samples;
    bool denoiser;
    std::filesystem::path outputPath;
    std::filesystem::path assetRootDir;
};

void renderOptixOffline(OptixOfflineConfig const& config);
