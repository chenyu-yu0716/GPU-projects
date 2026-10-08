#include <video_writer.h>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <common/timer.h>
#include <gpu/buffer.h>
#include <gpu/utility.h>

#include <mandelbrot_compute.h>

struct CommandLineOptions {
    int width{1280};
    int height{720};
    int framesPerSecond{30};
    bool antialias{true};
    std::filesystem::path outputPath{"mandelbrot.mp4"};

    // The camera animation mirrors the defaults of the online viewer.
    HighPrecisionComplex startCenter{HighPrecisionFloat{"-1.4184000000000000000000000000000000000000"},
                                     HighPrecisionFloat{0}};
    HighPrecisionFloat startZoom{1035};
    HighPrecisionComplex finalCenter{HighPrecisionFloat{"-1.416707803560595223063379502205564140068277553325999761"},
                                     HighPrecisionFloat{"0.000000000000000000000001192699352575212153707731000000"}};
    HighPrecisionFloat finalZoom{"1.6E22"};
};

static int parsePositiveInt(std::string_view value, std::string_view option) {
    int result = 0;
    auto const [pointer, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || pointer != value.data() + value.size() || result <= 0) {
        throw std::invalid_argument(std::string(option) + " must be a positive integer");
    }
    return result;
}

static CommandLineOptions parseCommandLine(int argc, char* argv[]) {
    CommandLineOptions options;
    for (int index = 1; index < argc;) {
        std::string_view const option(argv[index++]);
        if (index >= argc) {
            throw std::invalid_argument("Option is missing its value: " + std::string(option));
        }

        std::string_view const value(argv[index++]);
        if (option == "--width") {
            options.width = parsePositiveInt(value, option);
        }
        else if (option == "--height") {
            options.height = parsePositiveInt(value, option);
        }
        else if (option == "--fps") {
            options.framesPerSecond = parsePositiveInt(value, option);
        }
        else if (option == "--aa") {
            if (value == "0" || value == "false" || value == "1x1") {
                options.antialias = false;
            }
            else if (value == "1" || value == "true" || value == "2x2") {
                options.antialias = true;
            }
            else {
                throw std::invalid_argument(std::string(option) + " must be 0/false/1x1 or 1/true/2x2");
            }
        }
        else if (option == "--output") {
            options.outputPath = value;
        }
        else {
            throw std::invalid_argument("Unknown option: " + std::string(option));
        }
    }

    return options;
}

static void printUsage() {
    std::cout << "Usage:\n"
              << "  project1_video [options]\n\n"
              << "Renders the complete online camera animation to an H.264 video without\n"
              << "creating an OpenGL window. Requires ffmpeg on PATH.\n\n"
              << "Options:\n"
              << "  --width <pixels>     Video width, must be even (default: 1280)\n"
              << "  --height <pixels>    Video height, must be even (default: 720)\n"
              << "  --fps <frames>       Frames per second (default: 30)\n"
              << "  --aa <0|1>           0 = 1x1 sampling, 1 = 2x2 sampling (default: 1)\n"
              << "  --output <video.mp4> Output path (default: mandelbrot.mp4)\n";
}

static void selectCudaDevice() {
    int deviceCount = 0;
    cudaError_t const status = cudaGetDeviceCount(&deviceCount);
    if (status != cudaSuccess || deviceCount == 0) {
        throw std::runtime_error(
            std::format("No CUDA device is available for video rendering: {}", cudaGetErrorString(status)));
    }

    CHECK_CUDA(cudaSetDevice(0));
    gpu::printCudaDeviceName(0);
}

static unsigned char toByte(float value) {
    return static_cast<unsigned char>(std::lround(std::clamp(value, 0.0f, 1.0f) * 255.0f));
}

static void renderVideo(CommandLineOptions const& options) {
    selectCudaDevice();

    Palette palette;
    MandelbrotCompute compute(
        options.width, options.height, options.startCenter, options.startZoom, options.finalCenter, options.finalZoom);

    size_t const pixelCount = static_cast<size_t>(options.width) * static_cast<size_t>(options.height);
    gpu::Buffer<float4> output(pixelCount);
    std::vector<float4> renderedPixels(pixelCount);
    std::vector<unsigned char> rgba(pixelCount * 4);

    VideoWriter writer({
        .width = options.width,
        .height = options.height,
        .framesPerSecond = options.framesPerSecond,
        .outputPath = options.outputPath,
    });

    // The last frame lands exactly on the final camera.
    double const durationSeconds = MandelbrotCompute::durationSeconds();
    int const frameCount = static_cast<int>(std::lround(durationSeconds * options.framesPerSecond)) + 1;

    Timer timer;
    timer.tick();

    for (int frame = 0; frame < frameCount; ++frame) {
        double const timeSeconds = std::min(static_cast<double>(frame) / options.framesPerSecond, durationSeconds);
        MandelbrotCompute::FrameInfo const frameInfo =
            compute.render(output.data(), options.width, options.height, timeSeconds, options.antialias, palette);

        // The copy waits for the asynchronous kernel launched by render().
        CHECK_CUDA(cudaMemcpy(renderedPixels.data(), output.data(), output.bytes(), cudaMemcpyDeviceToHost));

        for (size_t index = 0; index < pixelCount; ++index) {
            float4 const pixel = renderedPixels[index];
            rgba[4 * index + 0] = toByte(pixel.x);
            rgba[4 * index + 1] = toByte(pixel.y);
            rgba[4 * index + 2] = toByte(pixel.z);
            rgba[4 * index + 3] = toByte(pixel.w);
        }

        writer.writeFrame(rgba.data());

        if (frame % options.framesPerSecond == 0 || frame + 1 == frameCount) {
            timer.tock();
            std::cout << std::format("Frame {}/{}  Time={:.1f}s  Zoom={:.4e}  Iter={}  Elapsed={:.1f}s\n",
                                     frame + 1,
                                     frameCount,
                                     timeSeconds,
                                     static_cast<double>(frameInfo.zoom),
                                     frameInfo.maxIterations,
                                     timer.getTotalTime())
                      << std::flush;
        }
    }

    writer.close();
    timer.tock();

    std::cout << std::format("Rendered {} frames in {:.1f} seconds\n", frameCount, timer.getTotalTime());
    std::cout << "Video saved to " << options.outputPath << '\n';
}

int main(int argc, char* argv[]) {
    try {
        bool helpRequested = false;
        for (int index = 1; index < argc; ++index) {
            helpRequested =
                helpRequested || std::string_view(argv[index]) == "--help" || std::string_view(argv[index]) == "-h";
        }
        if (helpRequested) {
            printUsage();
            return EXIT_SUCCESS;
        }

        CommandLineOptions const options = parseCommandLine(argc, argv);
        renderVideo(options);
    }
    catch (std::exception const& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unknown exception.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
