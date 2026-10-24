#pragma once

#include <gpu/utility.h>

#include <cstddef>
#include <cstdint>

namespace gpu {

class TextureCubemap {
public:
    enum class Face : uint32_t { PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ };

    struct Specification {
        size_t size{}; // Width and height of each square face.
        cudaChannelFormatDesc format{};
        cudaTextureDesc texDesc{};
        uint32_t arrayFlags{};
    };

public:
    explicit TextureCubemap(Specification const& specification);

    TextureCubemap(TextureCubemap&& other) noexcept;

    TextureCubemap& operator=(TextureCubemap&& other) noexcept;

    ~TextureCubemap() noexcept;

    cudaTextureObject_t texture() const noexcept {
        return m_texture;
    }

    cudaArray_t array() const noexcept {
        return m_array;
    }

    size_t size() const noexcept {
        return m_specification.size;
    }

    Specification const& spec() const noexcept {
        return m_specification;
    }

    void copyFromHost(Face face, void const* data, size_t rowPitchBytes);

    void copyFromDevice(Face face, void const* data, size_t rowPitchBytes);

private:
    Specification m_specification{};
    cudaArray_t m_array{};
    cudaTextureObject_t m_texture{};

private:
    void copyFrom(Face face, void const* data, size_t rowPitchBytes, cudaMemcpyKind kind);

    void destroy() noexcept;
};

} // namespace gpu
