#include <gpu/texture/texture2d.h>

#include <stdexcept>
#include <utility>

namespace gpu {

static size_t pixelBytes(cudaChannelFormatDesc const& desc) noexcept {
    return static_cast<size_t>(desc.x + desc.y + desc.z + desc.w) / 8;
}

Texture2D::Texture2D(Specification const& spec)
    : m_specification(spec) {
    if (!spec.height) {
        throw std::invalid_argument("Texture2D height must be positive");
    }
    CHECK_CUDA(cudaMallocArray(&m_array, &spec.format, spec.width, spec.height, spec.arrayFlags));

    cudaResourceDesc resource{};
    resource.resType = cudaResourceTypeArray;
    resource.res.array.array = m_array;
    cudaError_t const status = cudaCreateTextureObject(&m_texture, &resource, &spec.texDesc, nullptr);
    if (status != cudaSuccess) {
        LOG_CUDA(cudaFreeArray(std::exchange(m_array, nullptr)));
        checkCudaImpl(status, "cudaCreateTextureObject", __FILE__, __LINE__);
    }
}

Texture2D::~Texture2D() noexcept {
    destroy();
}

void Texture2D::destroy() noexcept {
    if (m_texture) {
        LOG_CUDA(cudaDestroyTextureObject(std::exchange(m_texture, 0)));
    }
    if (m_array) {
        LOG_CUDA(cudaFreeArray(std::exchange(m_array, nullptr)));
    }
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : m_specification(other.m_specification)
    , m_array(std::exchange(other.m_array, nullptr))
    , m_texture(std::exchange(other.m_texture, 0)) {}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept {
    if (this != &other) {
        destroy();
        m_specification = other.m_specification;
        m_array = std::exchange(other.m_array, nullptr);
        m_texture = std::exchange(other.m_texture, 0);
    }

    return *this;
}

void Texture2D::copyFrom(void const* data, size_t pitch, cudaMemcpyKind kind) {
    size_t const row = width() * pixelBytes(m_specification.format);
    CHECK_CUDA(cudaMemcpy2DToArray(m_array, 0, 0, data, pitch, row, height(), kind));
}

void Texture2D::copyFromHost(void const* data, size_t pitch) {
    copyFrom(data, pitch, cudaMemcpyHostToDevice);
}

void Texture2D::copyFromDevice(void const* data, size_t pitch) {
    copyFrom(data, pitch, cudaMemcpyDeviceToDevice);
}

} // namespace gpu
