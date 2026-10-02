#pragma once

#include <chrono>
#include <stdexcept>

class Timer {
public:
    void start() noexcept {
        m_startTime = Clock::now();
        m_started = true;
        m_stopped = false;
    }

    void stop() {
        if (!m_started) {
            throw std::logic_error("Timer must be started before it can be stopped");
        }

        m_stopTime = Clock::now();
        m_stopped = true;
    }

    [[nodiscard]] double elapsedMilliseconds() const {
        if (!m_started) {
            throw std::logic_error("Timer must be started before its elapsed time can be queried");
        }

        Clock::time_point const endTime = m_stopped ? m_stopTime : Clock::now();
        return std::chrono::duration<double, std::milli>(endTime - m_startTime).count();
    }

private:
    using Clock = std::chrono::steady_clock;

    Clock::time_point m_startTime{};
    Clock::time_point m_stopTime{};
    bool m_started{false};
    bool m_stopped{false};
};
