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
        , m_stopEvent{std::exchange(other.m_stopEvent, nullptr)}
        , m_totalTimeSeconds{std::exchange(other.m_totalTimeSeconds, 0.0f)} {}

    Timer& operator=(Timer&& other) noexcept {
        if (this != &other) {
            LOG_CUDA(cudaEventDestroy(m_stopEvent));
            LOG_CUDA(cudaEventDestroy(m_startEvent));

            m_startEvent = std::exchange(other.m_startEvent, nullptr);
            m_stopEvent = std::exchange(other.m_stopEvent, nullptr);
            m_totalTimeSeconds = std::exchange(other.m_totalTimeSeconds, 0.0f);
        }

        return *this;
    }

    void tick(cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaEventRecord(m_startEvent, stream));
    }

    float tock(cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaEventRecord(m_stopEvent, stream));
        CHECK_CUDA(cudaEventSynchronize(m_stopEvent));

        float elapsedMilliseconds = 0.0f;
        CHECK_CUDA(cudaEventElapsedTime(&elapsedMilliseconds, m_startEvent, m_stopEvent));

        const float elapsedSeconds = elapsedMilliseconds / 1000.0f;
        m_totalTimeSeconds += elapsedSeconds;

        // The stop event becomes the start event for the next interval.
        std::swap(m_startEvent, m_stopEvent);

        return elapsedSeconds;
    }

    [[nodiscard]] float getTotalTime() const noexcept {
        return m_totalTimeSeconds;
    }

private:
    cudaEvent_t m_startEvent{nullptr};
    cudaEvent_t m_stopEvent{nullptr};
    float m_totalTimeSeconds{0.0};
};

} // namespace gpu
