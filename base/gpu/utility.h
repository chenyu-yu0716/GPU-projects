#pragma once

#include <cuda_runtime.h>

#include <format>
#include <iostream>
#include <stdexcept>
#include <source_location>

#define LOG_CUDA(call) ::gpu::logCudaErrorImpl((call), #call)
#define CHECK_CUDA(call) ::gpu::checkCudaImpl((call), #call)

namespace gpu {

inline void logCudaErrorImpl(cudaError_t status, char const* cudaCall) {
    if (status != cudaSuccess) {
        std::cerr << std::format("[CUDA] {} failed({}): {}\n",
                                 cudaCall,
                                 static_cast<unsigned int>(status),
                                 cudaGetErrorString(cudaGetLastError()));
    }
}

inline void checkCudaImpl(cudaError_t status,
                          char const* cudaCall,
                          std::source_location const location = std::source_location::current()) {
    if (status != cudaSuccess) {
        std::cerr << std::format("[CUDA] {} failed at {}:{}: {}\n",
                                 cudaCall,
                                 location.file_name(),
                                 location.line(),
                                 cudaGetErrorString(cudaGetLastError()));

        throw std::runtime_error(
            std::format("CUDA runtime error({}): {}", static_cast<unsigned int>(status), cudaGetErrorName(status)));
    }
}

inline void printCudaDeviceName(int index) {
    cudaDeviceProp properties{};
    LOG_CUDA(cudaGetDeviceProperties(&properties, index));
    std::cout << std::format("Using CUDA device {}: {}\n", index, properties.name);
}

} // namespace gpu
