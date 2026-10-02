#pragma once

#include <memory>
#include <graphics/buffer.h>
#include <graphics/gl_program.h>
#include <graphics/vertex_array.h>
#include <common/window.h>
#include <gpu/graphics_resource/graphics_buffer_resource.h>

class Application {
public:
    Application(Window::Config const& config);

    void run();

    void handleInput();

    void renderFrame();

private:
    std::unique_ptr<Window> m_window;

    std::unique_ptr<gfx::Buffer> m_colorBuffer;
    std::unique_ptr<gpu::GraphicsBufferResource> m_colorBufferResource;

    std::unique_ptr<gfx::GLProgram> m_renderScreenProgram;
    std::unique_ptr<gfx::VertexArray> m_screenVertexArray;

private:
    size_t getColorBufferBytes() const noexcept;

    std::unique_ptr<gfx::GLProgram> createRenderScreenProgram();
};
