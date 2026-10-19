#pragma once

#include <optix/utility.h>

namespace optix {

class DeviceContext {
public:
    // Requires optixInit() and an active CUDA context on the calling thread.
    explicit DeviceContext(OptixDeviceContextOptions const* options = nullptr);

    ~DeviceContext() noexcept;

    DeviceContext(DeviceContext&& other) noexcept;

    DeviceContext& operator=(DeviceContext&& other) noexcept;

    OptixDeviceContext handle() const noexcept {
        return m_handle;
    }

private:
    OptixDeviceContext m_handle{};
};

} // namespace optix
