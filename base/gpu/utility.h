#pragma once

#include <cuda_runtime.h>

#include <cstdlib>
#include <format>
#include <iostream>
#include <source_location>

namespace gpu {

inline void checkCudaImpl(cudaError_t result,
                          char const* cudaCall,
                          std::source_location const location = std::source_location::current()) {
    if (result != cudaSuccess) {
        std::cerr << std::format("CUDA call {} failed at {}:{} in {}: {} ({})\n",
                                 cudaCall,
                                 location.file_name(),
                                 location.line(),
                                 location.function_name(),
                                 cudaGetErrorName(result),
                                 static_cast<unsigned int>(result));
        std::exit(EXIT_FAILURE);
    }
}

} // namespace gpu

#define CHECK_CUDA(call) ::gpu::checkCudaImpl((call), #call)
