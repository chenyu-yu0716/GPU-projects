#pragma once

#include <cuda_runtime.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <source_location>

#define LOG_CUDA(call) ::gpu::logCudaErrorImpl((call), #call)
#define CHECK_CUDA(call) ::gpu::checkCudaImpl((call), #call)

namespace gpu {

// Keep these helpers usable in .cu files: NVCC 13.2 with MSVC can fail during
// device code generation when std::format is instantiated in a called helper.
inline void logCudaErrorImpl(cudaError_t status, char const* cudaCall) {
    if (status != cudaSuccess) {
        std::cerr << "[CUDA] " << cudaCall << " failed(" << static_cast<unsigned int>(status)
                  << "): " << cudaGetErrorString(cudaGetLastError()) << '\n';
    }
}

inline void checkCudaImpl(cudaError_t status,
                          char const* cudaCall,
                          std::source_location const location = std::source_location::current()) {
    if (status != cudaSuccess) {
        std::cerr << "[CUDA] " << cudaCall << " failed at " << location.file_name() << ':' << location.line() << ": "
                  << cudaGetErrorString(cudaGetLastError()) << '\n';

        throw std::runtime_error(std::string("CUDA runtime error(") +
                                 std::to_string(static_cast<unsigned int>(status)) + "): " + cudaGetErrorName(status));
    }
}

inline void printCudaDeviceName(int index) {
    cudaDeviceProp properties{};
    LOG_CUDA(cudaGetDeviceProperties(&properties, index));
    std::cout << "Using CUDA device " << index << ": " << properties.name << '\n';
}

} // namespace gpu
