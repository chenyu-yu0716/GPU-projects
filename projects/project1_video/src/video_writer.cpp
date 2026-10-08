#include <video_writer.h>

#include <format>
#include <iostream>
#include <stdexcept>
#include <string>

#if defined(_WIN32)
static std::FILE* openPipe(std::string const& command) {
    return _popen(command.c_str(), "wb");
}

static int closePipe(std::FILE* pipe) {
    return _pclose(pipe);
}
#else
static std::FILE* openPipe(std::string const& command) {
    return popen(command.c_str(), "w");
}

static int closePipe(std::FILE* pipe) {
    return pclose(pipe);
}
#endif

VideoWriter::VideoWriter(Config const& config) {
    if (config.width <= 0 || config.height <= 0 || config.framesPerSecond <= 0) {
        throw std::invalid_argument("Video width, height, and frame rate must be positive");
    }
    // The yuv420p pixel format halves the chroma resolution in both directions.
    if (config.width % 2 != 0 || config.height % 2 != 0) {
        throw std::invalid_argument("Video width and height must be even");
    }

    std::string const outputPath = config.outputPath.string();
    if (outputPath.empty() || outputPath.find('"') != std::string::npos) {
        throw std::invalid_argument("Video output path must be non-empty and must not contain quotes");
    }

    std::string const command = std::format("ffmpeg -y -loglevel error -f rawvideo -pixel_format rgba"
                                            " -video_size {}x{} -framerate {} -i -"
                                            " -vf vflip -c:v libx264 -crf 18 -pix_fmt yuv420p \"{}\"",
                                            config.width,
                                            config.height,
                                            config.framesPerSecond,
                                            outputPath);

    m_pipe = openPipe(command);
    if (m_pipe == nullptr) {
        throw std::runtime_error("Failed to start ffmpeg; make sure it is installed and on PATH");
    }

    m_frameBytes = static_cast<size_t>(config.width) * static_cast<size_t>(config.height) * 4;
}

VideoWriter::~VideoWriter() {
    if (m_pipe != nullptr && closePipe(m_pipe) != 0) {
        std::cerr << "ffmpeg exited with an error while the video writer was destroyed\n";
    }
}

void VideoWriter::writeFrame(unsigned char const* rgba) {
    if (m_pipe == nullptr) {
        throw std::logic_error("Video writer is already closed");
    }

    if (std::fwrite(rgba, 1, m_frameBytes, m_pipe) != m_frameBytes) {
        throw std::runtime_error("Failed to send a frame to ffmpeg");
    }
}

void VideoWriter::close() {
    if (m_pipe == nullptr) {
        return;
    }

    int const status = closePipe(m_pipe);
    m_pipe = nullptr;
    if (status != 0) {
        throw std::runtime_error("ffmpeg failed to encode the video");
    }
}
