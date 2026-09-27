#include "graphics/graphics_context.h"

#include <algorithm>
#include <iostream>
#include <stdexcept>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "graphics/gl_utility.h"

// Prefer discrete GPU
#if defined(_WIN32)
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace gfx {
GraphicsContext::GraphicsContext(std::pair<int, int> const& minVersion)
    : m_version{std::max(minVersion, std::pair{4, 6})} {}

GraphicsContext::~GraphicsContext() {}

void GraphicsContext::init(GLFWwindow* window) {
    glfwMakeContextCurrent(window);

    // load OpenGL library functions
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("glad initialization OpenGL failure");
    }

    if (!checkVersion(window)) {
        throw std::runtime_error("Minimum OpenGL version does not satisfied");
    }

    std::cout << "OpenGL\n";
    std::cout << "+ version:     " << reinterpret_cast<char const*>(glGetString(GL_VERSION)) << '\n';
    std::cout << "+ renderer:    " << reinterpret_cast<char const*>(glGetString(GL_RENDERER)) << '\n';
    std::cout << "+ glsl:        " << reinterpret_cast<char const*>(glGetString(GL_SHADING_LANGUAGE_VERSION)) << '\n';

    GLint maxBlockSize{};
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &maxBlockSize);
    std::cout << "+ ubo limit:   " << maxBlockSize << " bytes\n";

    std::cout << "+ max texture\n";
    GLint maxVertexTextureUint{};
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureUint);
    std::cout << "  + vertex:    " << maxVertexTextureUint << '\n';

    GLint maxFragmentTextureUint{};
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxFragmentTextureUint);
    std::cout << "  + fragment:  " << maxFragmentTextureUint << '\n';

    GLint maxCombinedTextureUint{};
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUint);
    std::cout << "  + total:     " << maxCombinedTextureUint << '\n';

    std::cout << "+ max image\n";
    GLint maxVertexImageUint{};
    glGetIntegerv(GL_MAX_VERTEX_IMAGE_UNIFORMS, &maxVertexImageUint);
    std::cout << "  + vertex: " << maxVertexImageUint << '\n';

    GLint maxFragmentImageUint{};
    glGetIntegerv(GL_MAX_FRAGMENT_IMAGE_UNIFORMS, &maxFragmentImageUint);
    std::cout << "  + fragment: " << maxFragmentImageUint << '\n';

    GLint maxTextureBufferSize{};
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTextureBufferSize);
    std::cout << "+ texture buffer limit: " << maxTextureBufferSize << " bytes\n\n";

    // disable dither for performance
    glDisable(GL_DITHER);

    // always enable seamless cubemap sampling, to overcome cubemap edge sample artifacts
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    restoreDepthStates();

    restoreStencilStates();

#ifndef NDEBUG
    registerDebugOutput();
#endif
}

int GraphicsContext::getMajorVersion() const noexcept {
    return m_version.first;
}

int GraphicsContext::getMinorVersion() const noexcept {
    return m_version.second;
}

void GraphicsContext::restoreDepthStates() {
    // depth test is disabled by default
    glDisable(GL_DEPTH_TEST);
    // depth buffer clear value is 1.0f
    glClearDepth(1.0f);
    // depth value is writable
    glDepthMask(GL_TRUE);
    // depth compare function is less
    glDepthFunc(GL_LESS);
    // depth range is from [0.0f, 1.0f]
    glDepthRange(0.0f, 1.0f);
}

void GraphicsContext::restoreStencilStates() {
    // stencil test is disabled by default
    glDisable(GL_STENCIL_TEST);
    // stencil buffer clear value is 0
    glClearStencil(0);
    // all stencil bits are writable
    glStencilMask(0xFF);
    // stencil test is always, ref value is 0 and mask is OxFF
    // CompareFunc(StencilOp(ref, sValue) & mask)
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    // if CompareFunc tested is true, then keep the current value
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

bool GraphicsContext::checkVersion(GLFWwindow* window) {
    auto const majorVersion{glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MAJOR)};
    auto const minorVersion{glfwGetWindowAttrib(window, GLFW_CONTEXT_VERSION_MINOR)};

    if (majorVersion == 0) {
        return false;
    }

    if (majorVersion < m_version.first || (majorVersion == m_version.first && minorVersion < m_version.second)) {
        return false;
    }

    m_version.first = majorVersion;
    m_version.second = minorVersion;

    return true;
}

static void APIENTRY debugCallback(GLenum source,
                                   GLenum type,
                                   GLuint id,
                                   GLenum severity,
                                   GLsizei length,
                                   GLchar const* message,
                                   void const* userParam) {
    // ignore these non-significant error codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) {
        return;
    }

    std::string sourceStr;
    switch (source) {
    case GL_DEBUG_SOURCE_API:
        sourceStr = "API";
        break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
        sourceStr = "window system";
        break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER:
        sourceStr = "shader compiler";
        break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:
        sourceStr = "third party";
        break;
    case GL_DEBUG_SOURCE_APPLICATION:
        sourceStr = "application";
        break;
    case GL_DEBUG_SOURCE_OTHER:
        sourceStr = "other";
        break;
    }

    std::string typeStr;
    switch (type) {
    case GL_DEBUG_TYPE_ERROR:
        typeStr = "error";
        break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
        typeStr = "deprecated behaviour";
        break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
        typeStr = "undefined behaviour";
        break;
    case GL_DEBUG_TYPE_PORTABILITY:
        typeStr = "portability";
        break;
    case GL_DEBUG_TYPE_PERFORMANCE:
        typeStr = "performance";
        break;
    case GL_DEBUG_TYPE_MARKER:
        typeStr = "marker";
        break;
    case GL_DEBUG_TYPE_PUSH_GROUP:
        typeStr = "push group";
        break;
    case GL_DEBUG_TYPE_POP_GROUP:
        typeStr = "pop group";
        break;
    case GL_DEBUG_TYPE_OTHER:
        typeStr = "other";
        break;
    }

    std::string msg{message};
    switch (severity) {
    case GL_DEBUG_SEVERITY_HIGH:
        std::cerr << "OpenGL " << sourceStr << " (" << typeStr << "): " << msg << '\n';
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        std::cerr << "OpenGL " << sourceStr << " (" << typeStr << "): " << msg << '\n';
        break;
    case GL_DEBUG_SEVERITY_LOW:
        std::cout << "OpenGL " << sourceStr << " (" << typeStr << "): " << msg << '\n';
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        std::cout << "OpenGL " << sourceStr << " (" << typeStr << "): " << msg << '\n';
        break;
    }

#ifdef _MSC_VER
    if (type != GL_DEBUG_TYPE_MARKER && type != GL_DEBUG_TYPE_PERFORMANCE) {
        //__debugbreak();
    }
#endif
}

bool GraphicsContext::registerDebugOutput() {
    if (m_version.first < 4 || m_version.second < 3) {
        return false;
    }

    GLint flags{};
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT)) {
        return false;
    }

    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(debugCallback, nullptr);

    constexpr GLenum source{GL_DONT_CARE};
    constexpr GLenum type{GL_DONT_CARE};
    constexpr GLenum severity{GL_DONT_CARE};
    glDebugMessageControl(source, type, severity, 0, nullptr, GL_TRUE);

    checkGLErrors();

    return true;
}
} // namespace gfx
