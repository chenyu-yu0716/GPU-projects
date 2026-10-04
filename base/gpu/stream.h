#pragma once

#include <stdexcept>
#include <string>

#include <gpu/utility.h>

namespace gpu {

class Stream {
public:
    enum class Flags : uint32_t {
        Default = cudaStreamDefault,
        NonBlocking = cudaStreamNonBlocking,
    };

public:
    Stream() {
        CHECK_CUDA(cudaStreamCreate(&m_stream));
    }

    Stream(Flags flags) {
        CHECK_CUDA(cudaStreamCreateWithFlags(&m_stream, static_cast<uint32_t>(flags)));
    }

    Stream(Flags flags, int priority) {
        CHECK_CUDA(cudaStreamCreateWithPriority(&m_stream, static_cast<uint32_t>(flags), priority));
    }

    Stream(Stream&& other) noexcept
        : m_stream(other.m_stream) {
        other.m_stream = nullptr;
    }

    ~Stream() {
        LOG_CUDA(cudaStreamDestroy(m_stream));
    }

    Stream& operator=(Stream&& other) noexcept {
        if (this != &other) {
            LOG_CUDA(cudaStreamDestroy(m_stream));
            m_stream = other.m_stream;
            other.m_stream = nullptr;
        }

        return *this;
    }

    cudaStream_t get() const noexcept {
        return m_stream;
    }

    static bool tryGetPriorityRange(int& leastPriority, int& greatestPriority) noexcept {
        return cudaDeviceGetStreamPriorityRange(&leastPriority, &greatestPriority) == cudaSuccess;
    }

    static void getPriorityRange(int& leastPriority, int& greatestPriority) {
        CHECK_CUDA(cudaDeviceGetStreamPriorityRange(&leastPriority, &greatestPriority));
    }

private:
    cudaStream_t m_stream{nullptr};
};

} // namespace gpu