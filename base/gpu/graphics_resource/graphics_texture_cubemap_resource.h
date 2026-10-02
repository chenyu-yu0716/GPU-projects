#pragma once

#include "graphics/gl_utility.h"

#include <cuda_gl_interop.h>

#include "gpu/graphics_resource/graphics_resource.h"
#include "graphics/texture/texture_cubemap.h"

namespace gpu {

class GraphicsTextureCubemapResource final : public GraphicsResource {
public:
    explicit GraphicsTextureCubemapResource(gfx::TextureCubemap const& texture, RegisterFlags flags = None) {
        CHECK_CUDA(cudaGraphicsGLRegisterImage(
            getResourceAddress(), texture.getNativeHandle(), texture.getNativeType(), static_cast<uint32_t>(flags)));
    }

    GraphicsTextureCubemapResource(GraphicsTextureCubemapResource&&) noexcept = default;

    ~GraphicsTextureCubemapResource() = default;

    GraphicsTextureCubemapResource& operator=(GraphicsTextureCubemapResource&&) noexcept = default;

    cudaArray_t getMappedArray(gfx::TextureCubemap::Face face, uint32_t mipLevel = 0) const {
        cudaArray_t array{nullptr};
        CHECK_CUDA(cudaGraphicsSubResourceGetMappedArray(
            &array, getResource(), static_cast<uint32_t>(face), mipLevel));
        return array;
    }
};

} // namespace gpu
