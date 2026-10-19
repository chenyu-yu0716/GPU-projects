#include "graphics/transform_feedback.h"

#include <cassert>
#include <stdexcept>
#include <utility>
#include "graphics/gl_utility.h"

namespace gfx {

static GLenum toNativePrimitive(RHI::Primitive primitive) {
    switch (primitive) {
    case RHI::Primitive::Points:
        return GL_POINTS;
    case RHI::Primitive::Lines:
        return GL_LINES;
    case RHI::Primitive::LineStrip:
        return GL_LINE_STRIP;
    case RHI::Primitive::LineLoop:
        return GL_LINE_LOOP;
    case RHI::Primitive::Triangles:
        return GL_TRIANGLES;
    case RHI::Primitive::TriangleStrip:
        return GL_TRIANGLE_STRIP;
    case RHI::Primitive::TriangleFan:
        return GL_TRIANGLE_FAN;
    }

    throw std::logic_error("Unsupported transform feedback primitive type");
}

static GLenum toNativeTransformFeedbackPrimitive(RHI::Primitive primitive) {
    switch (primitive) {
    case RHI::Primitive::Points:
        return GL_POINTS;
    case RHI::Primitive::Lines:
    case RHI::Primitive::LineStrip:
    case RHI::Primitive::LineLoop:
        return GL_LINES;
    case RHI::Primitive::Triangles:
    case RHI::Primitive::TriangleStrip:
    case RHI::Primitive::TriangleFan:
        return GL_TRIANGLES;
    }

    throw std::logic_error("Unsupported transform feedback primitive type");
}

TransformFeedback::TransformFeedback() {
    glCreateTransformFeedbacks(1, &m_handle);
}

TransformFeedback::~TransformFeedback() {
    assert(!isActive() && "TransformFeedback must be ended before destruction");

    if (isValid()) {
        glDeleteTransformFeedbacks(1, &m_handle);
    }
}

TransformFeedback& TransformFeedback::operator=(TransformFeedback&& rhs) noexcept {
    if (this != &rhs) {
        assert(!isActive() && "The destination TransformFeedback must be ended before move assignment");

        if (isValid()) {
            glDeleteTransformFeedbacks(1, &m_handle);
        }

        RHIResource::operator=(std::move(rhs));
    }

    return *this;
}

void TransformFeedback::setBuffer(uint32_t binding, Buffer const& buffer) const {
    glTransformFeedbackBufferBase(m_handle, binding, buffer.getNativeHandle());
}

void TransformFeedback::setBufferRange(uint32_t binding, Buffer const& buffer, intptr_t offset, size_t size) const {
    glTransformFeedbackBufferRange(m_handle, binding, buffer.getNativeHandle(), GLintptr(offset), GLsizeiptr(size));
}

void TransformFeedback::bind() const {
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, m_handle);
}

void TransformFeedback::unbind() {
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
}

void TransformFeedback::begin(RHI::Primitive primitive) const {
    GLenum const mode{toNativeTransformFeedbackPrimitive(primitive)};
    glBeginTransformFeedback(mode);
}

void TransformFeedback::draw(RHI::Primitive primitive) const {
    glDrawTransformFeedback(toNativePrimitive(primitive), m_handle);
}

void TransformFeedback::drawInstanced(RHI::Primitive primitive, uint32_t instanceCount) const {
    glDrawTransformFeedbackInstanced(toNativePrimitive(primitive), m_handle, GLsizei(instanceCount));
}

bool TransformFeedback::isActive() const {
    if (!isValid()) {
        return false;
    }

    GLint active{GL_FALSE};
    glGetTransformFeedbackiv(m_handle, GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    return active == GL_TRUE;
}

void TransformFeedback::pause() {
    glPauseTransformFeedback();
}

void TransformFeedback::resume() {
    glResumeTransformFeedback();
}

void TransformFeedback::end() {
    glEndTransformFeedback();
}

} // namespace gfx
