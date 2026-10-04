#include <playback_controller.h>

#include <algorithm>

#include <common/event/keyboard_event.h>

PlaybackController::PlaybackController(double durationSeconds, bool antialias) noexcept
    : m_durationSeconds(durationSeconds)
    , m_antialias(antialias) {}

bool PlaybackController::handleKey(KeyboardEvent const& event) {
    switch (event.getKeyCode()) {
    case KeyCode::Escape:
        return true;
    case KeyCode::Space:
        m_playing = !m_playing;
        break;
    case KeyCode::R:
    case KeyCode::Home:
        m_timeSeconds = 0.0;
        m_playing = true;
        break;
    case KeyCode::End:
        m_timeSeconds = m_durationSeconds;
        m_playing = false;
        break;
    case KeyCode::Left:
        m_timeSeconds = std::max(0.0, m_timeSeconds - 2.0);
        break;
    case KeyCode::Right:
        m_timeSeconds = std::min(m_durationSeconds, m_timeSeconds + 2.0);
        break;
    case KeyCode::Up:
        m_speed = std::min(16.0, m_speed * 1.25);
        break;
    case KeyCode::Down:
        m_speed = std::max(0.0625, m_speed / 1.25);
        break;
    case KeyCode::A:
        m_antialias = !m_antialias;
        break;
    case KeyCode::LeftBracket:
        m_paletteShift -= 4.0f;
        break;
    case KeyCode::RightBracket:
        m_paletteShift += 4.0f;
        break;
    default:
        break;
    }
    return false;
}

void PlaybackController::update(double wallTimeSeconds) noexcept {
    if (!m_hasPreviousWallTime) {
        m_previousWallTime = wallTimeSeconds;
        m_hasPreviousWallTime = true;
        return;
    }

    double const elapsed = std::max(0.0, wallTimeSeconds - m_previousWallTime);
    m_previousWallTime = wallTimeSeconds;
    if (!m_playing) {
        return;
    }

    m_timeSeconds += elapsed * m_speed;
    if (m_timeSeconds >= m_durationSeconds) {
        m_timeSeconds = m_durationSeconds;
        m_playing = false;
    }
}

void PlaybackController::advance(double deltaTimeSeconds) noexcept {
    if (!m_playing) {
        return;
    }

    m_timeSeconds += std::max(0.0, deltaTimeSeconds) * m_speed;
    if (m_timeSeconds >= m_durationSeconds) {
        m_timeSeconds = m_durationSeconds;
        m_playing = false;
    }
}

double PlaybackController::timeSeconds() const noexcept {
    return m_timeSeconds;
}

double PlaybackController::durationSeconds() const noexcept {
    return m_durationSeconds;
}

double PlaybackController::speed() const noexcept {
    return m_speed;
}

bool PlaybackController::playing() const noexcept {
    return m_playing;
}

bool PlaybackController::antialias() const noexcept {
    return m_antialias;
}

float PlaybackController::paletteShift() const noexcept {
    return m_paletteShift;
}
