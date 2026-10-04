#pragma once

#include <cstddef>

#include "graphics/gl_utility.h"

#include <cuda_gl_interop.h>

#include "graphics/buffer.h"
#include "gpu/graphics_resource/graphics_resource.h"

namespace gpu {

class GraphicsBufferResource final : public GraphicsResource {
public:
    GraphicsBufferResource(gfx::Buffer const& buffer, RegisterFlags flags) {
        CHECK_CUDA(
            cudaGraphicsGLRegisterBuffer(getResourceAddress(), buffer.getNativeHandle(), static_cast<uint32_t>(flags)));
    }

    GraphicsBufferResource(GraphicsBufferResource&&) noexcept = default;

    ~GraphicsBufferResource() = default;

    GraphicsBufferResource& operator=(GraphicsBufferResource&&) noexcept = default;

    void* getMappedPointer(size_t* size = nullptr) const {
        void* pointer{nullptr};
        size_t mappedSize{0};
        CHECK_CUDA(cudaGraphicsResourceGetMappedPointer(&pointer, &mappedSize, getResource()));

        if (size != nullptr) {
            *size = mappedSize;
        }

        return pointer;
    }

    template <typename T> T* getMappedPointer(size_t* count = nullptr) const {
        size_t size{0};
        auto* pointer = static_cast<T*>(getMappedPointer(&size));

        if (count != nullptr) {
            *count = size / sizeof(T);
        }

        return pointer;
    }
};

} // namespace gpu
