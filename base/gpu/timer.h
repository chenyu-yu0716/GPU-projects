#pragma once

#include <cuda_runtime.h>

#include <utility>

#include "gpu/utility.h"

namespace gpu {

class Timer {
public:
    Timer() {
        CHECK_CUDA(cudaEventCreate(&m_startEvent));
        CHECK_CUDA(cudaEventCreate(&m_stopEvent));
    }

    ~Timer() {
        LOG_CUDA(cudaEventDestroy(m_stopEvent));
        LOG_CUDA(cudaEventDestroy(m_startEvent));
    }

    Timer(Timer&& other) noexcept
        : m_startEvent{std::exchange(other.m_startEvent, nullptr)}
        , m_stopEvent{std::exchange(other.m_stopEvent, nullptr)} {}

    Timer& operator=(Timer&& other) noexcept {
        if (this != &other) {
            LOG_CUDA(cudaEventDestroy(m_stopEvent));
            LOG_CUDA(cudaEventDestroy(m_startEvent));

            m_startEvent = std::exchange(other.m_startEvent, nullptr);
            m_stopEvent = std::exchange(other.m_stopEvent, nullptr);
        }

        return *this;
    }

    void start(cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaEventRecord(m_startEvent, stream));
    }

    void stop(cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaEventRecord(m_stopEvent, stream));
        CHECK_CUDA(cudaEventSynchronize(m_stopEvent));
    }

    [[nodiscard]] double elapsedMilliseconds() const {
        float elapsedMilliseconds = 0.0f;
        CHECK_CUDA(cudaEventElapsedTime(&elapsedMilliseconds, m_startEvent, m_stopEvent));

        return elapsedMilliseconds;
    }

private:
    cudaEvent_t m_startEvent{nullptr};
    cudaEvent_t m_stopEvent{nullptr};
};

} // namespace gpu
