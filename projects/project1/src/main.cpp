#include <application.h>

int main(int argc, char* argv[]) {
    Window::Config config{
        .title = "Mandelbrot Renderer",
        .width = 640,
        .height = 360,
        .vsync = true,
        .resizable = true,
        .fullscreen = false,
        .maximize = false,
        .msaa = true,
        .driverVersion = { 4, 5 },
    };

    try {
        Application app(config);
        app.run();
    }
    catch (std::runtime_error& e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unknown error\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
