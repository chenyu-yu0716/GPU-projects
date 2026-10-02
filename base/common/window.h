#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>

struct GLFWwindow;

namespace gfx {
class GraphicsContext;
}

class Window {
public:
    using FramebufferSizeCallback = std::function<void(int width, int height)>;
    using MouseButtonCallback = std::function<void(int button, int action, int mods)>;
    using CursorPositionCallback = std::function<void(double x, double y)>;
    using ScrollCallback = std::function<void(double xoffset, double yoffset)>;
    using KeyCallback = std::function<void(int keycode, int scancode, int action, int mods)>;
    using CharacterCallback = std::function<void(unsigned int codepoint)>;

    struct Config {
        std::string title;
        uint32_t width;
        uint32_t height;
        bool vsync;
        bool resizable;
        bool fullscreen;
        bool maximize;
        bool msaa;
        std::pair<int, int> driverVersion;
    };

public:
    explicit Window(Config const& config);

    ~Window();

    Window(Window const&) = delete;

    Window& operator=(Window const&) = delete;

    GLFWwindow* handle() const noexcept {
        return m_handle;
    }

    std::string const& title() const noexcept {
        return m_title;
    }

    uint32_t width() const noexcept {
        return m_width;
    }

    uint32_t height() const noexcept {
        return m_height;
    }

    uint32_t framebufferWidth() const noexcept {
        return m_framebufferWidth;
    }

    uint32_t framebufferHeight() const noexcept {
        return m_framebufferHeight;
    }

    bool isVSync() const noexcept {
        return m_vsync;
    }

    void setVSync(bool enabled);

    void setResizable(bool resizable) const;

    void maximize();

    void centerAlign();

    bool shouldClose() const;

    void requestClose() const;

    void close() const;

    void pollEvents() const;

    void swapBuffers() const;

    void registerFramebufferSizeCallback(FramebufferSizeCallback callback);

    void registerMouseButtonCallback(MouseButtonCallback callback);

    void registerCursorPositionCallback(CursorPositionCallback callback);

    void registerScrollCallback(ScrollCallback callback);

    void registerKeyCallback(KeyCallback callback);

    void registerCharacterCallback(CharacterCallback callback);

private:
    GLFWwindow* m_handle = nullptr;
    std::unique_ptr<gfx::GraphicsContext> m_graphicsContext;

    std::string m_title;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_framebufferWidth = 0;
    uint32_t m_framebufferHeight = 0;
    bool m_vsync = false;

    FramebufferSizeCallback m_framebufferSizeCallback;
    MouseButtonCallback m_mouseButtonCallback;
    CursorPositionCallback m_cursorPositionCallback;
    ScrollCallback m_scrollCallback;
    KeyCallback m_keyCallback;
    CharacterCallback m_characterCallback;

    static int s_instanceCount;

private:
    void destroyNativeWindow();

    static void windowSizeCallback(GLFWwindow* handle, int width, int height);

    static void framebufferSizeCallback(GLFWwindow* handle, int width, int height);

    static void mouseButtonCallback(GLFWwindow* handle, int button, int action, int mods);

    static void cursorPositionCallback(GLFWwindow* handle, double x, double y);

    static void scrollCallback(GLFWwindow* handle, double xoffset, double yoffset);

    static void keyCallback(GLFWwindow* handle, int keycode, int scancode, int action, int mods);

    static void characterCallback(GLFWwindow* handle, unsigned int codepoint);
};
