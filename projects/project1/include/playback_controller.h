#pragma once

class KeyboardEvent;

class PlaybackController {
public:
    explicit PlaybackController(double durationSeconds, bool antialias = false) noexcept;

    // Returns true only for the quit command; all other recognized commands
    // update local playback state.
    bool handleKey(KeyboardEvent const& event);

    void update(double wallTimeSeconds) noexcept;

    void advance(double deltaTimeSeconds) noexcept;

    double timeSeconds() const noexcept;

    double durationSeconds() const noexcept;

    double speed() const noexcept;

    bool playing() const noexcept;

    bool antialias() const noexcept;

    float paletteShift() const noexcept;

private:
    double m_durationSeconds{0.0};
    double m_timeSeconds{0.0};
    double m_speed{1.0};
    double m_previousWallTime{0.0};
    bool m_hasPreviousWallTime{false};
    bool m_playing{false};
    bool m_antialias{false};
    float m_paletteShift{0.0f};
};
