#pragma once

#include <cstdint>
#include <utility>

#include "common/enum.h"
#include "gpu/utility.h"

namespace gpu {

// Owns one CUDA resource registered from an OpenGL object. The OpenGL object
// must outlive this wrapper and must not be deleted while the resource is mapped.
class GraphicsResource {
public:
    enum RegisterFlags : uint32_t {
        None = cudaGraphicsRegisterFlagsNone,
        ReadOnly = cudaGraphicsRegisterFlagsReadOnly,
        WriteDiscard = cudaGraphicsRegisterFlagsWriteDiscard,
        SurfaceLoadStore = cudaGraphicsRegisterFlagsSurfaceLoadStore,
        TextureGather = cudaGraphicsRegisterFlagsTextureGather,
    };

    void map(cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaGraphicsMapResources(1, &m_resource, stream));
    }

    void unmap(cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaGraphicsUnmapResources(1, &m_resource, stream));
    }

protected:
    GraphicsResource() noexcept = default;

    GraphicsResource(GraphicsResource&& rhs) noexcept
        : m_resource{std::exchange(rhs.m_resource, nullptr)} {}

    GraphicsResource& operator=(GraphicsResource&& rhs) noexcept {
        if (this != &rhs) {
            LOG_CUDA(cudaGraphicsUnregisterResource(m_resource));
            m_resource = std::exchange(rhs.m_resource, nullptr);
        }

        return *this;
    }

    ~GraphicsResource() noexcept {
        LOG_CUDA(cudaGraphicsUnregisterResource(m_resource));
    }

    cudaGraphicsResource_t getResource() const noexcept {
        return m_resource;
    }

    cudaGraphicsResource_t* getResourceAddress() noexcept {
        return &m_resource;
    }

private:
    cudaGraphicsResource_t m_resource{nullptr};
};

ENABLE_BITMASK_OPERATION(GraphicsResource::RegisterFlags);

} // namespace gpu
