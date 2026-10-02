#pragma once

#include <cuda_gl_interop.h>

#include "gpu/graphics_resource/graphics_resource.h"
#include "graphics/texture/texture2d_array.h"

namespace gpu {

class GraphicsTexture2DArrayResource final : public GraphicsResource {
public:
    explicit GraphicsTexture2DArrayResource(gfx::Texture2DArray const& texture, RegisterFlags flags = None) {
        CHECK_CUDA(cudaGraphicsGLRegisterImage(
            getResourceAddress(), texture.getNativeHandle(), texture.getNativeType(), static_cast<uint32_t>(flags)));
    }

    GraphicsTexture2DArrayResource(GraphicsTexture2DArrayResource&&) noexcept = default;

    ~GraphicsTexture2DArrayResource() = default;

    GraphicsTexture2DArrayResource& operator=(GraphicsTexture2DArrayResource&&) noexcept = default;

    cudaArray_t getMappedArray(uint32_t layer, uint32_t mipLevel = 0) const {
        cudaArray_t array{nullptr};
        CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray(&array, getResource(), layer, mipLevel));
        return array;
    }
};

} // namespace gpu
