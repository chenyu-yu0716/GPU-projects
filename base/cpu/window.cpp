#include "window.h"

#include "../graphics/graphics_context.h"

#include <algorithm>
#include <array>
#include <format>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

#include <glad/glad.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

Window::Window(Config const& config)
    : m_title(std::move(config.title))
    , m_width(config.width)
    , m_height(config.height)
    , m_framebufferWidth(config.width)
    , m_framebufferHeight(config.height) {
    if (config.width > std::numeric_limits<int>::max() || config.height > std::numeric_limits<int>::max()) {
        throw std::invalid_argument("Window dimensions exceed GLFW's supported range");
    }

    bool const isFirstWindow{s_instanceCount == 0};
    if (isFirstWindow) {
        glfwSetErrorCallback([](int error, char const* description) {
            std::cerr << std::format("GLFW error {}: {}\n", error, description);
        });

        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
    }

    glfwDefaultWindowHints();

    glfwWindowHint(GLFW_RESIZABLE, config.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_SAMPLES, config.msaa ? 4 : 0);

    // Set up window hint based on monitor
    GLFWmonitor* monitor{nullptr};
    if (config.fullscreen || config.maximize) {
        monitor = glfwGetPrimaryMonitor();

        if (monitor == nullptr) {
            if (isFirstWindow) {
                glfwTerminate();
            }
            throw std::runtime_error("glfwGetPrimaryMonitor failure");
        }

        auto const videoMode{glfwGetVideoMode(monitor)};
        if (videoMode == nullptr) {
            if (isFirstWindow) {
                glfwTerminate();
            }
            throw std::runtime_error("glfwGetVideoMode failure");
        }

        if (config.fullscreen) {
            glfwWindowHint(GLFW_DECORATED, false);
            glfwWindowHint(GLFW_RED_BITS, videoMode->redBits);
            glfwWindowHint(GLFW_GREEN_BITS, videoMode->greenBits);
            glfwWindowHint(GLFW_BLUE_BITS, videoMode->blueBits);
            glfwWindowHint(GLFW_REFRESH_RATE, videoMode->refreshRate);
        }
        else {
            monitor = nullptr;
            glfwWindowHint(GLFW_DECORATED, true);
            glfwWindowHint(GLFW_MAXIMIZED, true);
        }

        m_width = videoMode->width;
        m_height = videoMode->height;
    }

    // Since OpenGL context is managed by GLFW, we need to specify window hints for
    // correct graphic context creation.
    m_graphicsContext = std::make_unique<gfx::GraphicsContext>(config.driverVersion);

    // Create GLFW window
    m_handle = glfwCreateWindow(int(m_width), int(m_height), m_title.c_str(), monitor, nullptr);
    if (m_handle == nullptr) {
        if (isFirstWindow) {
            glfwTerminate();
        }
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Get actual window size and framebuffer size
    int width, height;
    glfwGetWindowSize(m_handle, &width, &height);
    m_width = static_cast<uint32_t>(width);
    m_height = static_cast<uint32_t>(height);

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(m_handle, &framebufferWidth, &framebufferHeight);
    m_framebufferWidth = static_cast<uint32_t>(framebufferWidth);
    m_framebufferHeight = static_cast<uint32_t>(framebufferHeight);

    // Init graphics context
    try {
        m_graphicsContext->init(m_handle);
    }
    catch (...) {
        destroyNativeWindow();
        if (isFirstWindow) {
            glfwTerminate();
        }
        throw;
    }

    // Set window related attributes
    setVSync(config.vsync);
    setResizable(config.resizable);

    if (glfwRawMouseMotionSupported()) {
        glfwSetInputMode((GLFWwindow*)m_handle, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
    }

    // Set callback functions
    glfwSetWindowUserPointer(m_handle, this);
    glfwSetWindowSizeCallback(m_handle, windowSizeCallback);
    glfwSetFramebufferSizeCallback(m_handle, framebufferSizeCallback);

    // Increment instance count to track GLFW initialization and termination
    ++s_instanceCount;
}

Window::~Window() {
    destroyNativeWindow();
    if (s_instanceCount > 0 && --s_instanceCount == 0) {
        glfwTerminate();
    }
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_handle) != 0;
}

void Window::requestClose() const {
    glfwSetWindowShouldClose(m_handle, GLFW_TRUE);
}

void Window::close() const {
    requestClose();
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::swapBuffers() const {
    glfwSwapBuffers(m_handle);
}

void Window::setVSync(bool enabled) {
    if (enabled) {
        glfwSwapInterval(1);
    }
    else {
        glfwSwapInterval(0);
    }

    m_vsync = enabled;
}

void Window::setResizable(bool resizable) const {
    glfwSetWindowAttrib((GLFWwindow*)m_handle, GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);
}

void Window::maximize() {
    glfwMaximizeWindow((GLFWwindow*)m_handle);
}

void Window::centerAlign() {
    auto const videoMode{glfwGetVideoMode(glfwGetPrimaryMonitor())};
    auto const xpos{0.5f * (videoMode->width - m_width)};
    auto const ypos{0.5f * (videoMode->height - m_height)};

    glfwSetWindowPos((GLFWwindow*)m_handle, xpos, ypos);
}

void Window::registerFramebufferSizeCallback(FramebufferSizeCallback callback) {
    m_framebufferSizeCallback = std::move(callback);
}

void Window::registerMouseButtonCallback(MouseButtonCallback callback) {
    m_mouseButtonCallback = std::move(callback);
    glfwSetMouseButtonCallback(m_handle, m_mouseButtonCallback ? mouseButtonCallback : nullptr);
}

void Window::registerCursorPositionCallback(CursorPositionCallback callback) {
    m_cursorPositionCallback = std::move(callback);
    glfwSetCursorPosCallback(m_handle, m_cursorPositionCallback ? cursorPositionCallback : nullptr);
}

void Window::registerScrollCallback(ScrollCallback callback) {
    m_scrollCallback = std::move(callback);
    glfwSetScrollCallback(m_handle, m_scrollCallback ? scrollCallback : nullptr);
}

void Window::registerKeyCallback(KeyCallback callback) {
    m_keyCallback = std::move(callback);
    glfwSetKeyCallback(m_handle, m_keyCallback ? keyCallback : nullptr);
}

void Window::registerCharacterCallback(CharacterCallback callback) {
    m_characterCallback = std::move(callback);
    glfwSetCharCallback(m_handle, m_characterCallback ? characterCallback : nullptr);
}

void Window::windowSizeCallback(GLFWwindow* handle, int width, int height) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && width >= 0 && height >= 0) {
        window->m_width = static_cast<uint32_t>(width);
        window->m_height = static_cast<uint32_t>(height);
    }
}

void Window::framebufferSizeCallback(GLFWwindow* handle, int width, int height) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && width >= 0 && height >= 0) {
        window->m_framebufferWidth = static_cast<uint32_t>(width);
        window->m_framebufferHeight = static_cast<uint32_t>(height);
        if (window->m_framebufferSizeCallback) {
            window->m_framebufferSizeCallback(width, height);
        }
    }
}

void Window::mouseButtonCallback(GLFWwindow* handle, int button, int action, int mods) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && window->m_mouseButtonCallback) {
        window->m_mouseButtonCallback(button, action, mods);
    }
}

void Window::cursorPositionCallback(GLFWwindow* handle, double x, double y) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && window->m_cursorPositionCallback) {
        window->m_cursorPositionCallback(x, y);
    }
}

void Window::scrollCallback(GLFWwindow* handle, double xoffset, double yoffset) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && window->m_scrollCallback) {
        window->m_scrollCallback(xoffset, yoffset);
    }
}

void Window::keyCallback(GLFWwindow* handle, int keycode, int scancode, int action, int mods) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && window->m_keyCallback) {
        window->m_keyCallback(keycode, scancode, action, mods);
    }
}

void Window::characterCallback(GLFWwindow* handle, unsigned int codepoint) {
    auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
    if (window != nullptr && window->m_characterCallback) {
        window->m_characterCallback(codepoint);
    }
}

void Window::destroyNativeWindow() {
    if (m_handle != nullptr) {
        glfwDestroyWindow(m_handle);
        m_handle = nullptr;
    }
}
