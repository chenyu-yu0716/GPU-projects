#include <complex.hpp>
#include <common/window.h>
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

    Application app(config);

    app.run();

    return 0;
}
