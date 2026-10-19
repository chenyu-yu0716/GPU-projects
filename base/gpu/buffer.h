#pragma once

#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <utility>

#include "gpu/utility.h"

namespace gpu {

template <typename T>
class Buffer {
public:
    Buffer() noexcept = default;

    explicit Buffer(size_t count) {
        allocate(count);
    }

    Buffer(Buffer&& other) noexcept
        : m_data{std::exchange(other.m_data, nullptr)}
        , m_size{std::exchange(other.m_size, 0)} {}

    ~Buffer() noexcept {
        LOG_CUDA(cudaFree(m_data));
    }

    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            LOG_CUDA(cudaFree(m_data));
            m_data = std::exchange(other.m_data, nullptr);
            m_size = std::exchange(other.m_size, 0);
        }

        return *this;
    }

    explicit operator bool() const noexcept {
        return m_data != nullptr;
    }

    T* data() noexcept {
        return m_data;
    }

    T const* data() const noexcept {
        return m_data;
    }

    size_t size() const noexcept {
        return m_size;
    }

    size_t sizeBytes() const noexcept {
        return m_size * sizeof(T);
    }

    bool empty() const noexcept {
        return m_size == 0;
    }

    void reallocate(size_t count) {
        if (count == m_size) {
            return;
        }

        Buffer temporary;
        temporary.allocate(count);
        swap(temporary);
    }

    void copyFromHost(T const* src) {
        CHECK_CUDA(cudaMemcpy(m_data, src, sizeBytes(), cudaMemcpyHostToDevice));
    }

    void copyFromHost(std::span<T const> src) {
        CHECK_CUDA(cudaMemcpy(m_data, src.data(), src.size_bytes(), cudaMemcpyHostToDevice));
    }

    void copyToHost(T* dst) {
        CHECK_CUDA(cudaMemcpy(dst, m_data, sizeBytes(), cudaMemcpyDeviceToHost));
    }

    void copyToHost(std::span<T> dst) {
        CHECK_CUDA(cudaMemcpy(dst.data(), m_data, dst.size_bytes(), cudaMemcpyDeviceToHost));
    }

    static Buffer adopt(T* dptr, size_t count) noexcept {
        return Buffer{dptr, count};
    }

    T* release() noexcept {
        m_size = 0;
        return std::exchange(m_data, nullptr);
    }

    void swap(Buffer& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
    }

    friend void swap(Buffer& a, Buffer& b) noexcept {
        a.swap(b);
    }

private:
    Buffer(T* dptr, size_t count) noexcept
        : m_data{dptr}
        , m_size{count} {}

    void allocate(size_t count) {
        if (count == 0) {
            return;
        }

        if (count > std::numeric_limits<size_t>::max() / sizeof(T)) {
            throw std::length_error("CUDA buffer size overflows size_t");
        }

        CHECK_CUDA(cudaMalloc(&m_data, count * sizeof(T)));
        m_size = count;
    }

private:
    T* m_data{nullptr};
    size_t m_size{0};
};

} // namespace gpu
