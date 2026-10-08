#pragma once

#include <cstddef>
#include <cstdio>
#include <filesystem>

// Streams raw RGBA frames to an ffmpeg child process that encodes them as
// H.264. Frames are expected bottom row first, as the CUDA renderer writes
// them, and are flipped by ffmpeg.
class VideoWriter {
public:
    struct Config {
        int width{0};
        int height{0};
        int framesPerSecond{0};
        std::filesystem::path outputPath;
    };

public:
    // Throws std::invalid_argument for an unsupported configuration and
    // std::runtime_error if ffmpeg cannot be started.
    explicit VideoWriter(Config const& config);

    VideoWriter(VideoWriter const&) = delete;

    ~VideoWriter();

    VideoWriter& operator=(VideoWriter const&) = delete;

    // `rgba` must hold width * height * 4 bytes.
    void writeFrame(unsigned char const* rgba);

    // Waits for ffmpeg to finish encoding. Throws std::runtime_error if it failed.
    void close();

private:
    std::FILE* m_pipe{nullptr};
    size_t m_frameBytes{0};
};
