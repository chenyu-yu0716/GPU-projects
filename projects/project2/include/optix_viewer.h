#pragma once

#include <camera.h>
#include <camera_controller.h>
#include <common/application.h>
#include <gpu/graphics_resource/graphics_buffer_resource.h>
#include <graphics/buffer.h>
#include <graphics/gl_program.h>
#include <graphics/vertex_array.h>
#include <optix/device_context.h>
#include <optix_renderer.h>

#include <cstdint>
#include <memory>

class OptixViewer final : public Application {
public:
    explicit OptixViewer(Application::Config const& config);

    ~OptixViewer() override;

private:
    // Declared in dependency order: renderer and scene are destroyed before the context.
    std::unique_ptr<optix::DeviceContext> m_context;
    std::unique_ptr<Scene> m_scene;
    std::unique_ptr<OptixRenderer> m_renderer;

    // OpenGL output storage and its CUDA interop registration.
    std::unique_ptr<gfx::Buffer> m_outputBuffer;
    std::unique_ptr<gpu::GraphicsBufferResource> m_outputResource;
    uint32_t m_outputWidth{};
    uint32_t m_outputHeight{};

    // Full-screen presentation resources.
    std::unique_ptr<gfx::GLProgram> m_renderScreenProgram;
    std::unique_ptr<gfx::VertexArray> m_screenVertexArray;

    // Camera pose and its input controller.
    Camera m_camera{{12.48607f, 1.925702f, 1.317638f}, glm::quat(1, 0, 0, 0)};
    CameraController m_cameraController{{0, 3, 0}};

    // Progressive sample count and denoiser toggle state.
    uint32_t m_samples{0};
    bool m_denoiserEnabled{true};

private:
    void handleEvent(Event& event) override;

    void updateCamera();

    void renderFrame() override;

    void resizeOutput(uint32_t width, uint32_t height);

    void updateWindowTitle();
};
