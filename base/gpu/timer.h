#pragma once

#include <cuda_runtime.h>

#include <cassert>
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
        destroyEvents();
    }

    Timer(Timer&& other) noexcept
        : m_startEvent{std::exchange(other.m_startEvent, nullptr)}
        , m_stopEvent{std::exchange(other.m_stopEvent, nullptr)}
        , m_started{std::exchange(other.m_started, false)}
        , m_stopped{std::exchange(other.m_stopped, false)} {}

    Timer& operator=(Timer&& other) noexcept {
        if (this != &other) {
            destroyEvents();

            m_startEvent = std::exchange(other.m_startEvent, nullptr);
            m_stopEvent = std::exchange(other.m_stopEvent, nullptr);
            m_started = std::exchange(other.m_started, false);
            m_stopped = std::exchange(other.m_stopped, false);
        }

        return *this;
    }

    void start(cudaStream_t stream = nullptr) {
        CHECK_CUDA(cudaEventRecord(m_startEvent, stream));
        m_started = true;
        m_stopped = false;
    }

    void stop(cudaStream_t stream = nullptr) {
        assert(m_started && "Timer must be started before it can be stopped");

        CHECK_CUDA(cudaEventRecord(m_stopEvent, stream));
        CHECK_CUDA(cudaEventSynchronize(m_stopEvent));
        m_stopped = true;
    }

    [[nodiscard]] double elapsedMilliseconds() const {
        assert(m_started && m_stopped &&
               "GPU timer must be started and stopped before its elapsed time can be queried");

        float elapsedMilliseconds = 0.0f;
        CHECK_CUDA(cudaEventElapsedTime(&elapsedMilliseconds, m_startEvent, m_stopEvent));
        return elapsedMilliseconds;
    }

private:
    void destroyEvents() noexcept {
        if (m_stopEvent != nullptr) {
            cudaEventDestroy(m_stopEvent);
        }
        if (m_startEvent != nullptr) {
            cudaEventDestroy(m_startEvent);
        }
    }

    cudaEvent_t m_startEvent{nullptr};
    cudaEvent_t m_stopEvent{nullptr};
    bool m_started{false};
    bool m_stopped{false};
};

} // namespace gpu
