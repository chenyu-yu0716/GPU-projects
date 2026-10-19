#include <optix/device_context.h>
#include <optix_function_table_definition.h>

#include <iostream>
#include <utility>

namespace optix {

namespace {
void logCallback(uint32_t level, char const* tag, char const* message, void*) noexcept {
    try {
        std::cerr << "[OptiX][" << level << "][" << (tag ? tag : "") << "] " << (message ? message : "") << '\n';
    }
    catch (...) {
        // Exceptions must not cross the OptiX C callback boundary.
    }
}
} // namespace

DeviceContext::DeviceContext(OptixDeviceContextOptions const* options) {
    // A null CUDA context binds OptiX to the CUDA context active on this thread.
    CHECK_OPTIX(optixDeviceContextCreate(nullptr, options, &m_handle));
    OptixResult const result = optixDeviceContextSetLogCallback(m_handle, logCallback, nullptr, 4);
    if (result != OPTIX_SUCCESS) {
        LOG_OPTIX(optixDeviceContextDestroy(m_handle));
        m_handle = nullptr;
        checkOptixImpl(result, "optixDeviceContextSetLogCallback", __FILE__, __LINE__);
    }
}

DeviceContext::DeviceContext(DeviceContext&& other) noexcept
    : m_handle(std::exchange(other.m_handle, nullptr)) {}

DeviceContext::~DeviceContext() noexcept {
    if (m_handle) {
        LOG_OPTIX(optixDeviceContextDestroy(m_handle));
    }
}

DeviceContext& DeviceContext::operator=(DeviceContext&& other) noexcept {
    if (this != &other) {
        if (m_handle) {
            LOG_OPTIX(optixDeviceContextDestroy(m_handle));
        }

        m_handle = std::exchange(other.m_handle, nullptr);
    }

    return *this;
}

} // namespace optix
