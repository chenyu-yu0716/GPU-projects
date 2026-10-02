#include <application.h>
#include <graphics/rhi.h>

#include <mandelbrot.h>

#include <gpu/timer.h>

Application::Application(Window::Config const& config) {
    m_window = std::make_unique<Window>(config);

    m_colorBuffer = std::make_unique<gfx::Buffer>(
        gfx::Buffer::UsageFlags::ShaderStorageBuffer,
        getColorBufferBytes(),
        gfx::Buffer::StorageFlags::Dynamic);

    m_colorBufferResource = std::make_unique<gpu::GraphicsBufferResource>(
        *m_colorBuffer, gpu::GraphicsResource::RegisterFlags::WriteDiscard);

    m_renderScreenProgram = createRenderScreenProgram();
    m_screenVertexArray = std::make_unique<gfx::VertexArray>();
}

void Application::run() {
    while (!m_window->shouldClose()) {
        m_window->pollEvents();
        m_window->swapBuffers();
    }
}

void Application::handleInput() {
    // Handle user input
}

void Application::renderFrame() {

    m_colorBufferResource->map();
    float3* dPixels = m_colorBufferResource->getMappedPointer<float3>();

    // launch the CUDA kernel to compute the Mandelbrot set


    m_colorBufferResource->unmap();


    // draw a full-screen triangle to render the screen
    // 1. bind the shader program and set the uniform variable for the width of the screen
    m_renderScreenProgram->use();
    constexpr int uWidthLocation = 0;
    m_renderScreenProgram->setUniform(uWidthLocation, m_window->width());

    // 2. bind the color buffer as a shader storage buffer
    m_colorBuffer->bindAsShaderStorage(0);

    // 3. set the uniform variable for the width of the screen
    m_screenVertexArray->bind();
    RHI::drawArrays(RHI::PrimitiveType::Triangles, 0, 3);

    // 4. cleanup bindings
    m_screenVertexArray->unbind();
    m_colorBuffer->unbindAsShaderStorage(0);
    m_renderScreenProgram->unuse();
}

size_t Application::getColorBufferBytes() const {
    return m_window->width() * m_window->height() * sizeof(float) * 3;
}

std::unique_ptr<gfx::GLProgram> Application::createRenderScreenProgram() {
    const char* vertexShaderCode = R"GLSL(
#version 450 core

const vec2 triangle[3] = vec2[3](
    vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

void main() {
    gl_Position = vec4(triangle[gl_VertexID], 0.0, 1.0);
}
)GLSL";

    const char* fragementShaderCode = R"GLSL(
#version 450 core

layout(location = 0) uniform uint uWidth;
layout(std430, binding = 0) readonly buffer Pixels { vec3 pixels[]; };

out vec4 fragColor;

void main() {
    uint index = uint(gl_FragCoord.y) * uWidth + uint(gl_FragCoord.x);
    fragColor = vec4(pixels[index], 1.0);
}
)GLSL";

    return std::make_unique<gfx::GLProgram>(std::vector<gfx::ShaderModule>{
        gfx::ShaderModule(vertexShaderCode, gfx::ShaderModule::Stage::Vertex),
        gfx::ShaderModule(fragementShaderCode, gfx::ShaderModule::Stage::Fragment)
    });
}
