#pragma once

#include <cuda_gl_interop.h>

#include "gpu/graphics_resource/graphics_resource.h"
#include "graphics/texture/texture2d.h"

namespace gpu {

class GraphicsTexture2DResource final : public GraphicsResource {
public:
    explicit GraphicsTexture2DResource(gfx::Texture2D const& texture, RegisterFlags flags) {
        CHECK_CUDA(cudaGraphicsGLRegisterImage(
            getResourceAddress(), texture.getNativeHandle(), texture.getNativeType(), static_cast<uint32_t>(flags)));
    }

    GraphicsTexture2DResource(GraphicsTexture2DResource&&) noexcept = default;

    ~GraphicsTexture2DResource() = default;

    GraphicsTexture2DResource& operator=(GraphicsTexture2DResource&&) noexcept = default;

    cudaArray_t getMappedArray(uint32_t mipLevel = 0) const {
        cudaArray_t array{nullptr};
        CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray(&array, getResource(), 0, mipLevel));
        return array;
    }
};

} // namespace gpu
