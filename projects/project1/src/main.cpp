#include <mandelbrot_offline_render.h>

#ifndef PROJECT1_OFFLINE_ONLY
#include <mandelbrot_viewer.h>
#endif

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

struct CommandLineOptions {
    bool offline{false};
    int width{640};
    int height{360};
    bool antialias{false};
    HighPrecisionComplex center{HighPrecisionFloat{"-1.416707803560595223063379502205564140068277553325999761"},
                                HighPrecisionFloat{"0.000000000000000000000001192699352575212153707731000000"}};
    HighPrecisionFloat zoom{"1.6E22"};
    std::filesystem::path outputPath;
};

static int parseInt(std::string_view value, std::string_view option) {
    int result = 0;
    auto const [pointer, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || pointer != value.data() + value.size()) {
        throw std::invalid_argument(std::string(option) + " must be an integer");
    }
    return result;
}

static HighPrecisionFloat parseHighPrecisonFloat(std::string_view value, std::string_view option) {
    try {
        return HighPrecisionFloat(value.data());
    }
    catch (std::exception const&) {
        throw std::invalid_argument(std::string(option) + " must be a valid decimal value");
    }
}

static CommandLineOptions parseCommandLine(int argc, char* argv[]) {
    CommandLineOptions options;
    for (int index = 1; index < argc; ++index) {
        if (std::string_view(argv[index]) == "--offline") {
            options.offline = true;
        }
    }
    if (options.offline) {
        options.width = 1920;
        options.height = 1080;
        options.antialias = true;
    }

    for (int index = 1; index < argc;) {
        std::string_view const option(argv[index++]);
        if (option == "--offline") {
            continue;
        }
        if (index >= argc) {
            throw std::invalid_argument("Option is missing its value: " + std::string(option));
        }

        std::string_view const value(argv[index++]);
        if (option == "--re" || option == "--center-re") {
            options.center.re = parseHighPrecisonFloat(value, option);
        }
        else if (option == "--im" || option == "--center-im") {
            options.center.im = parseHighPrecisonFloat(value, option);
        }
        else if (option == "--zoom") {
            options.zoom = parseHighPrecisonFloat(value, option);
            if (options.zoom <= HighPrecisionFloat(0)) {
                throw std::invalid_argument("--zoom must be greater than zero");
            }
        }
        else if (option == "--width") {
            options.width = parseInt(value, option);
            if (options.width <= 0) {
                throw std::invalid_argument("--width must be positive");
            }
        }
        else if (option == "--height") {
            options.height = parseInt(value, option);
            if (options.height <= 0) {
                throw std::invalid_argument("--height must be positive");
            }
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

    if (options.offline && options.outputPath.empty()) {
        options.outputPath = options.center.re.convert_to<std::string>() + "+" +
                             options.center.im.convert_to<std::string>() + "i_" +
                             options.zoom.convert_to<std::string>() + "z.png";
    }

    return options;
}

static OfflineRenderConfig makeOfflineConfig(CommandLineOptions const& options) {
    return OfflineRenderConfig{
        .center = options.center,
        .zoom = options.zoom,
        .width = options.width,
        .height = options.height,
        .antialias = options.antialias,
        .outputPath = options.outputPath,
    };
}

#ifndef PROJECT1_OFFLINE_ONLY
static MandelbrotViewer::Config makeOnScreenConfig(CommandLineOptions const& options) {
    return MandelbrotViewer::Config{
        .applicationConfig =
            {
                .windowConfig =
                    {
                        .title = "Mandelbrot Renderer",
                        .width = static_cast<uint32_t>(options.width),
                        .height = static_cast<uint32_t>(options.height),
                        .vsync = true,
                        .resizable = false,
                        .fullscreen = false,
                        .maximize = false,
                        .msaa = false,
                        .driverVersion = {4, 5},
                    },
                .assetRootDir = {},
            },
        .antialias = options.antialias,
        .center = options.center,
        .zoom = options.zoom,
    };
}
#endif

static void printUsage() {
#ifdef PROJECT1_OFFLINE_ONLY
    std::cout << "Usage:\n"
              << "  project1 --offline [options]\n\n"
              << "This build only supports offline rendering.\n\n"
              << "Options:\n";
#else
    std::cout << "Usage:\n"
              << "  project1 [options]\n"
              << "  project1 --offline [options]\n\n"
              << "Rendering mode:\n"
              << "  default       Interactive OpenGL rendering (online).\n"
              << "  --offline     Render one RGBA PNG without creating an OpenGL window.\n\n"
              << "Options:\n";
#endif
    std::cout << "  --re <decimal>       Real coordinate (alias: --center-re)\n"
              << "  --im <decimal>       Imaginary coordinate (alias: --center-im)\n"
              << "  --zoom <decimal>     Zoom\n"
              << "  --width <pixels>     Image/window width\n"
              << "  --height <pixels>    Image/window height\n"
              << "  --aa <0|1>            0 = 1x1 sampling, 1 = 2x2 sampling\n"
              << "  --output <image.png>  Offline output path\n\n"
#ifdef PROJECT1_OFFLINE_ONLY
              << "Offline defaults: 1920x1080, AA 1.\n"
#else
              << "Online defaults: 640x360, AA 0.\n"
              << "Offline defaults: 1920x1080, AA 1.\n"
#endif
              << "The default re/im/zoom are the application's final camera values.\n";
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
#ifdef PROJECT1_OFFLINE_ONLY
        if (!options.offline) {
            throw std::invalid_argument("This build only supports offline rendering; pass --offline");
        }
#endif
        if (options.offline) {
            OfflineRenderConfig const config = makeOfflineConfig(options);
            renderMandelbrotSetOffline(config);
        }
#ifndef PROJECT1_OFFLINE_ONLY
        else {
            MandelbrotViewer app(makeOnScreenConfig(options));
            app.run();
        }
#endif
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
