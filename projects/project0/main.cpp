// CUDA Runtime API device-property query, modelled after NVIDIA's deviceQuery
// sample.  It intentionally performs no CUDA work beyond querying each device.

#include <cuda_runtime.h>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace {

void checkCuda(cudaError_t status, const char *operation) {
    if (status == cudaSuccess) {
        return;
    }

    std::fprintf(stderr, "%s failed: %s\n", operation, cudaGetErrorString(status));
    std::exit(EXIT_FAILURE);
}

int coresPerMultiprocessor(int major, int minor) {
    // CUDA cores per SM for NVIDIA architectures supported by CUDA 12.x.
    struct SmToCores {
        int major;
        int minor;
        int cores;
    };
    constexpr SmToCores kMappings[] = {
        {2, 0, 32}, {2, 1, 48}, {3, 0, 192}, {3, 5, 192}, {3, 7, 192},
        {5, 0, 128}, {5, 2, 128}, {5, 3, 128}, {6, 0, 64}, {6, 1, 128},
        {6, 2, 128}, {7, 0, 64}, {7, 2, 64}, {7, 5, 64}, {8, 0, 64},
        {8, 6, 128}, {8, 7, 128}, {8, 9, 128}, {9, 0, 128},
    };

    for (const SmToCores &mapping : kMappings) {
        if (mapping.major == major && mapping.minor == minor) {
            return mapping.cores;
        }
    }
    return 0;
}

const char *yesNo(int value) { return value ? "Yes" : "No"; }

const char *computeModeName(int mode) {
    switch (mode) {
        case cudaComputeModeDefault:
            return "Default (multiple host threads can use ::cudaSetDevice() with this device simultaneously)";
        case cudaComputeModeExclusive:
            return "Exclusive (only one host thread in one process is able to use ::cudaSetDevice() with this device)";
        case cudaComputeModeProhibited:
            return "Prohibited (no host thread can use ::cudaSetDevice() with this device)";
        case cudaComputeModeExclusiveProcess:
            return "Exclusive Process (many threads in one process are able to use ::cudaSetDevice() with this device)";
        default:
            return "Unknown";
    }
}

void printDevice(int device) {
    cudaDeviceProp property{};
    checkCuda(cudaSetDevice(device), "cudaSetDevice");
    checkCuda(cudaGetDeviceProperties(&property, device), "cudaGetDeviceProperties");

    int driverVersion = 0;
    int runtimeVersion = 0;
    checkCuda(cudaDriverGetVersion(&driverVersion), "cudaDriverGetVersion");
    checkCuda(cudaRuntimeGetVersion(&runtimeVersion), "cudaRuntimeGetVersion");

    std::printf("\nDevice %d: \"%s\"\n", device, property.name);
    std::printf("  CUDA Driver Version / Runtime Version          %d.%d / %d.%d\n",
                driverVersion / 1000, (driverVersion % 100) / 10,
                runtimeVersion / 1000, (runtimeVersion % 100) / 10);
    std::printf("  CUDA Capability Major/Minor version number:    %d.%d\n", property.major, property.minor);
    std::printf("  Total amount of global memory:                 %.0f MBytes (%llu bytes)\n",
                static_cast<double>(property.totalGlobalMem) / (1024.0 * 1024.0),
                static_cast<unsigned long long>(property.totalGlobalMem));

    const int coresPerSm = coresPerMultiprocessor(property.major, property.minor);
    if (coresPerSm != 0) {
        std::printf("  (%03d) Multiprocessors, (%03d) CUDA Cores/MP:    %d CUDA Cores\n",
                    property.multiProcessorCount, coresPerSm,
                    coresPerSm * property.multiProcessorCount);
    } else {
        std::printf("  (%03d) Multiprocessors, CUDA Cores/MP:          Unknown\n",
                    property.multiProcessorCount);
    }

    int clockRate = 0;
    int memoryClockRate = 0;
    int gpuOverlap = 0;
    int kernelExecTimeout = 0;
    int computeMode = 0;
    checkCuda(cudaDeviceGetAttribute(&clockRate, cudaDevAttrClockRate, device), "cudaDevAttrClockRate");
    checkCuda(cudaDeviceGetAttribute(&memoryClockRate, cudaDevAttrMemoryClockRate, device),
              "cudaDevAttrMemoryClockRate");
    checkCuda(cudaDeviceGetAttribute(&gpuOverlap, cudaDevAttrGpuOverlap, device), "cudaDevAttrGpuOverlap");
    checkCuda(cudaDeviceGetAttribute(&kernelExecTimeout, cudaDevAttrKernelExecTimeout, device),
              "cudaDevAttrKernelExecTimeout");
    checkCuda(cudaDeviceGetAttribute(&computeMode, cudaDevAttrComputeMode, device),
              "cudaDevAttrComputeMode");

    std::printf("  GPU Max Clock rate:                            %.0f MHz (%.2f GHz)\n",
                clockRate * 1e-3, clockRate * 1e-6);
    std::printf("  Memory Clock rate:                             %.0f MHz\n", memoryClockRate * 1e-3);
    std::printf("  Memory Bus Width:                              %d-bit\n", property.memoryBusWidth);
    if (property.l2CacheSize != 0) {
        std::printf("  L2 Cache Size:                                 %d bytes\n", property.l2CacheSize);
    }
    std::printf("  Maximum Texture Dimension Size (x,y,z)         1D=(%d), 2D=(%d, %d), 3D=(%d, %d, %d)\n",
                property.maxTexture1D, property.maxTexture2D[0], property.maxTexture2D[1],
                property.maxTexture3D[0], property.maxTexture3D[1], property.maxTexture3D[2]);
    std::printf("  Maximum Layered 1D Texture Size, (num) layers  1D=(%d), %d layers\n",
                property.maxTexture1DLayered[0], property.maxTexture1DLayered[1]);
    std::printf("  Maximum Layered 2D Texture Size, (num) layers  2D=(%d, %d), %d layers\n",
                property.maxTexture2DLayered[0], property.maxTexture2DLayered[1],
                property.maxTexture2DLayered[2]);
    std::printf("  Total amount of constant memory:               %zu bytes\n", property.totalConstMem);
    std::printf("  Total amount of shared memory per block:       %zu bytes\n", property.sharedMemPerBlock);
    std::printf("  Total shared memory per multiprocessor:        %zu bytes\n", property.sharedMemPerMultiprocessor);
    std::printf("  Total number of registers available per block: %d\n", property.regsPerBlock);
    std::printf("  Warp size:                                     %d\n", property.warpSize);
    std::printf("  Maximum number of threads per multiprocessor:  %d\n", property.maxThreadsPerMultiProcessor);
    std::printf("  Maximum number of threads per block:           %d\n", property.maxThreadsPerBlock);
    std::printf("  Max dimension size of a thread block (x,y,z): (%d, %d, %d)\n",
                property.maxThreadsDim[0], property.maxThreadsDim[1], property.maxThreadsDim[2]);
    std::printf("  Max dimension size of a grid size    (x,y,z): (%d, %d, %d)\n",
                property.maxGridSize[0], property.maxGridSize[1], property.maxGridSize[2]);
    std::printf("  Maximum memory pitch:                          %zu bytes\n", property.memPitch);
    std::printf("  Texture alignment:                             %zu bytes\n", property.textureAlignment);
    std::printf("  Concurrent copy and kernel execution:          %s with %d copy engine(s)\n",
                yesNo(gpuOverlap), property.asyncEngineCount);
    std::printf("  Run time limit on kernels:                     %s\n", yesNo(kernelExecTimeout));
    std::printf("  Integrated GPU sharing Host Memory:            %s\n", yesNo(property.integrated));
    std::printf("  Support host page-locked memory mapping:       %s\n", yesNo(property.canMapHostMemory));
    std::printf("  Alignment requirement for Surfaces:            %s\n", yesNo(property.surfaceAlignment));
    std::printf("  Device has ECC support:                        %s\n", property.ECCEnabled ? "Enabled" : "Disabled");
#if defined(_WIN32)
    std::printf("  CUDA Device Driver Mode (TCC or WDDM):         %s\n",
                property.tccDriver ? "TCC (Tesla Compute Cluster Driver)" : "WDDM (Windows Display Driver Model)");
#endif
    std::printf("  Device supports Unified Addressing (UVA):      %s\n", yesNo(property.unifiedAddressing));
    std::printf("  Device supports Managed Memory:                %s\n", yesNo(property.managedMemory));
    std::printf("  Device supports Compute Preemption:            %s\n", yesNo(property.computePreemptionSupported));
    std::printf("  Supports Cooperative Kernel Launch:            %s\n", yesNo(property.cooperativeLaunch));
    std::printf("  Supports MultiDevice Co-op Kernel Launch:      %s\n", yesNo(property.cooperativeMultiDeviceLaunch));
    std::printf("  Device PCI Domain ID / Bus ID / location ID:   %d / %d / %d\n",
                property.pciDomainID, property.pciBusID, property.pciDeviceID);
    std::printf("  Compute Mode:\n     < %s >\n", computeModeName(computeMode));
}

void printPeerAccess(int deviceCount) {
    if (deviceCount < 2) {
        return;
    }

    for (int source = 0; source < deviceCount; ++source) {
        cudaDeviceProp sourceProperty{};
        checkCuda(cudaGetDeviceProperties(&sourceProperty, source), "cudaGetDeviceProperties");
        for (int destination = 0; destination < deviceCount; ++destination) {
            if (source == destination) {
                continue;
            }
            int canAccessPeer = 0;
            checkCuda(cudaDeviceCanAccessPeer(&canAccessPeer, source, destination), "cudaDeviceCanAccessPeer");
            cudaDeviceProp destinationProperty{};
            checkCuda(cudaGetDeviceProperties(&destinationProperty, destination), "cudaGetDeviceProperties");
            std::printf("> Peer access from %s (GPU%d) -> %s (GPU%d) : %s\n",
                        sourceProperty.name, source, destinationProperty.name, destination,
                        yesNo(canAccessPeer));
        }
    }
}

}  // namespace

int main(int argc, char **argv) {
    std::printf("%s Starting...\n\n", argc > 0 ? argv[0] : "deviceQuery");
    std::printf(" CUDA Device Query (Runtime API) version (CUDART static linking)\n\n");

    int deviceCount = 0;
    const cudaError_t status = cudaGetDeviceCount(&deviceCount);
    if (status != cudaSuccess) {
        std::printf("cudaGetDeviceCount returned %d\n-> %s\n", static_cast<int>(status), cudaGetErrorString(status));
        std::printf("Result = FAIL\n");
        return EXIT_FAILURE;
    }

    if (deviceCount == 0) {
        std::printf("There are no available device(s) that support CUDA\n");
    } else {
        std::printf("Detected %d CUDA Capable device(s)\n", deviceCount);
    }
    for (int device = 0; device < deviceCount; ++device) {
        printDevice(device);
    }
    printPeerAccess(deviceCount);

    int driverVersion = 0;
    int runtimeVersion = 0;
    checkCuda(cudaDriverGetVersion(&driverVersion), "cudaDriverGetVersion");
    checkCuda(cudaRuntimeGetVersion(&runtimeVersion), "cudaRuntimeGetVersion");
    std::printf("\ndeviceQuery, CUDA Driver = CUDART, CUDA Driver Version = %d.%d, "
                "CUDA Runtime Version = %d.%d, NumDevs = %d\n",
                driverVersion / 1000, (driverVersion % 100) / 10,
                runtimeVersion / 1000, (runtimeVersion % 100) / 10, deviceCount);
    std::printf("Result = PASS\n");
    return EXIT_SUCCESS;
}
