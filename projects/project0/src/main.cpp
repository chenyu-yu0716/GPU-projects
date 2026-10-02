#include <cuda_runtime.h>

#include <common/utility.h>
#include <device_info.h>

#include <cstdlib>

int main() {
    println("Device Query Starting...\n");
    println(" CUDA Device Query (Runtime API) version (CUDART static linking)\n");

    int deviceCount = 0;
    cudaError_t const error = cudaGetDeviceCount(&deviceCount);
    if (error != cudaSuccess) {
        println("cudaGetDeviceCount returned {}", static_cast<int>(error));
        println("-> {}", cudaGetErrorString(error));
        return EXIT_FAILURE;
    }
    if (deviceCount == 0) {
        println("There are no available device(s) that support CUDA");
        return EXIT_FAILURE;
    }

    println("Detected {} CUDA Capable device(s)\n", deviceCount);

    for (int device = 0; device < deviceCount; ++device) {
        printDeviceInfo(device);
    }

    printPeerAccessInfo(deviceCount);

    return EXIT_SUCCESS;
}
