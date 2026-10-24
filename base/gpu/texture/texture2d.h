#pragma once

#include <gpu/utility.h>

#include <cstddef>
#include <cstdint>

namespace gpu {

class Texture2D {
public:
    struct Specification {
        size_t width{};
        size_t height{};
        cudaChannelFormatDesc format{};
        cudaTextureDesc texDesc{};
        uint32_t arrayFlags{};
    };

public:
    explicit Texture2D(Specification const& spec);

    ~Texture2D() noexcept;

    Texture2D(Texture2D&& other) noexcept;

    Texture2D& operator=(Texture2D&& other) noexcept;

    cudaTextureObject_t texture() const noexcept {
        return m_texture;
    }

    cudaArray_t array() const noexcept {
        return m_array;
    }

    size_t width() const noexcept {
        return m_specification.width;
    }

    size_t height() const noexcept {
        return m_specification.height;
    }

    Specification const& spec() const noexcept {
        return m_specification;
    }

    void copyFromHost(void const* data, size_t rowPitchBytes);

    void copyFromDevice(void const* data, size_t rowPitchBytes);

private:
    Specification m_specification{};
    cudaArray_t m_array{};
    cudaTextureObject_t m_texture{};

private:
    void copyFrom(void const* data, size_t rowPitchBytes, cudaMemcpyKind kind);

    void destroy() noexcept;
};

} // namespace gpu
