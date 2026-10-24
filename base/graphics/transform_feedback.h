#pragma once

#include <cstddef>
#include <cstdint>

#include "graphics/buffer.h"
#include "graphics/rhi.h"
#include "graphics/rhi_resource.h"

namespace gfx {

class TransformFeedback : public RHIResource {
public:
    TransformFeedback();

    TransformFeedback(TransformFeedback&& rhs) noexcept = default;

    ~TransformFeedback();

    TransformFeedback& operator=(TransformFeedback&& rhs) noexcept;

    void setBuffer(uint32_t binding, Buffer const& buffer) const;

    void setBufferRange(uint32_t binding, Buffer const& buffer, intptr_t offset, size_t size) const;

    void bind() const;

    static void unbind();

    void draw(RHI::Primitive primitive) const;

    void drawInstanced(RHI::Primitive primitive, uint32_t instanceCount) const;

    // Paused transform feedback is still active.
    bool isActive() const;

    // The following operations act on the currently bound TransformFeedback object.
    void begin(RHI::Primitive primitive) const;

    static void pause();

    static void resume();

    static void end();
};

} // namespace gfx
