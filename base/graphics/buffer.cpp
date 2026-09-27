#include "graphics/buffer.h"

#include <stdexcept>
#include "graphics/gl_utility.h"

namespace gfx {
static GLenum toNativeUsage(Buffer::Usage usage) {
    switch (usage) {
    case Buffer::Usage::StaticDraw:
        return GL_STATIC_DRAW;
    case Buffer::Usage::DynamicDraw:
        return GL_DYNAMIC_DRAW;
    case Buffer::Usage::StreamDraw:
        return GL_STREAM_DRAW;
    case Buffer::Usage::StaticRead:
        return GL_STATIC_READ;
    case Buffer::Usage::DynamicRead:
        return GL_DYNAMIC_READ;
    case Buffer::Usage::StreamRead:
        return GL_STREAM_READ;
    case Buffer::Usage::StaticCopy:
        return GL_STATIC_COPY;
    case Buffer::Usage::DynamicCopy:
        return GL_DYNAMIC_COPY;
    case Buffer::Usage::StreamCopy:
        return GL_STATIC_COPY;
    default:
        throw std::logic_error("Unsupported usage");
    }

    return 0;
}

static GLenum toNativeAccess(Buffer::Access access) {
    switch (access) {
    case Buffer::Access::ReadOnly:
        return GL_READ_ONLY;
    case Buffer::Access::WriteOnly:
        return GL_WRITE_ONLY;
    case Buffer::Access::ReadWrite:
        return GL_READ_WRITE;
    default:
        throw std::logic_error("Unsupported access");
    }

    return 0;
}

static GLbitfield toNativeStorageFlags(Buffer::StorageFlags flags) noexcept {
    GLbitfield bitfields{0u};
    if (testBitmaskContain(flags, Buffer::StorageFlags::Dynamic)) {
        bitfields |= GL_DYNAMIC_STORAGE_BIT;
    }

    if (testBitmaskContain(flags, Buffer::StorageFlags::MapRead)) {
        bitfields |= GL_MAP_READ_BIT;
    }

    if (testBitmaskContain(flags, Buffer::StorageFlags::MapWrite)) {
        bitfields |= GL_MAP_WRITE_BIT;
    }

    if (testBitmaskContain(flags, Buffer::StorageFlags::MapPersistent)) {
        bitfields |= GL_MAP_PERSISTENT_BIT;
    }

    if (testBitmaskContain(flags, Buffer::StorageFlags::MapCoherent)) {
        bitfields |= GL_MAP_COHERENT_BIT;
    }

    if (testBitmaskContain(flags, Buffer::StorageFlags::Client)) {
        bitfields |= GL_CLIENT_STORAGE_BIT;
    }

    return bitfields;
}

static GLbitfield toNativeMapFlags(Buffer::MapFlags flags) noexcept {
    GLbitfield bitfields{0u};
    if (testBitmaskContain(flags, Buffer::MapFlags::Read)) {
        bitfields |= GL_MAP_READ_BIT;
    }

    if (testBitmaskContain(flags, Buffer::MapFlags::Write)) {
        bitfields |= GL_MAP_WRITE_BIT;
    }

    if (testBitmaskContain(flags, Buffer::MapFlags::Persistent)) {
        bitfields |= GL_MAP_PERSISTENT_BIT;
    }

    if (testBitmaskContain(flags, Buffer::MapFlags::Coherent)) {
        bitfields |= GL_MAP_COHERENT_BIT;
    }

    if (testBitmaskContain(flags, Buffer::MapFlags::InvalidateRange)) {
        bitfields |= GL_MAP_INVALIDATE_RANGE_BIT;
    }

    if (testBitmaskContain(flags, Buffer::MapFlags::InvalidateBuffer)) {
        bitfields |= GL_MAP_INVALIDATE_BUFFER_BIT;
    }

    if (testBitmaskContain(flags, Buffer::MapFlags::FlushExplicit)) {
        bitfields |= GL_MAP_FLUSH_EXPLICIT_BIT;
    }

    if (testBitmaskContain(flags, Buffer::MapFlags::Unsynchronized)) {
        bitfields |= GL_MAP_UNSYNCHRONIZED_BIT;
    }

    return bitfields;
}

Buffer::Buffer(UsageFlags usageFlags, size_t size, Usage usage, void const* data)
    : m_usageFlags{usageFlags}, m_size{size}, m_immutable{false} {
    glCreateBuffers(1, &m_handle);
    glNamedBufferData(m_handle, size, data, toNativeUsage(usage));
}

Buffer::Buffer(UsageFlags usageFlags, size_t size, StorageFlags storageFlags, void const* data)
    : m_usageFlags{usageFlags}, m_size{size}, m_immutable{true} {
    glCreateBuffers(1, &m_handle);
    glNamedBufferStorage(m_handle, size, data, toNativeStorageFlags(storageFlags));
}

Buffer::Buffer(Buffer&& rhs) noexcept
    : m_usageFlags{rhs.m_usageFlags}, m_size{rhs.m_size}, m_immutable{rhs.m_immutable} {
    rhs.m_size = 0;
}

Buffer::~Buffer() {
    if (m_handle) {
        glDeleteBuffers(1, &m_handle);
    }
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept {
    if (this != &rhs) {
        if (isValid()) {
            glDeleteBuffers(1, &m_handle);
        }

        m_size = rhs.m_size;
        const_cast<UsageFlags&>(m_usageFlags) = rhs.m_usageFlags;
        const_cast<bool&>(m_immutable) = rhs.m_immutable;

        RHIResource::operator=(std::move(rhs));
    }

    return *this;
}

void Buffer::resize(size_t size, Usage usage) {
    if (m_immutable) {
        throw std::logic_error("Cannot resize immutable buffer");
    }

    glNamedBufferData(m_handle, size, nullptr, toNativeUsage(usage));
    m_size = size;
}

void Buffer::read(void* data) const {
    glGetNamedBufferSubData(m_handle, 0, GLsizeiptr(m_size), data);
}

void Buffer::read(int offset, size_t size, void* data) const {
    glGetNamedBufferSubData(m_handle, GLintptr(offset), GLsizeiptr(size), data);
}

void Buffer::write(void const* data) const {
    glNamedBufferSubData(m_handle, 0, GLsizeiptr(m_size), data);
}

void Buffer::write(int offset, size_t size, void const* data) const {
    glNamedBufferSubData(m_handle, GLintptr(offset), GLsizeiptr(size), data);
}

void* Buffer::map(Access access) const {
    return glMapNamedBuffer(m_handle, toNativeAccess(access));
}

void Buffer::unmap() const {
    glUnmapNamedBuffer(m_handle);
}

void* Buffer::mapRange(intptr_t offset, size_t size, MapFlags mapFlags) const {
    return glMapNamedBufferRange(m_handle, offset, size, toNativeMapFlags(mapFlags));
}

void Buffer::flushMappedRange(intptr_t offset, size_t size) const {
    glFlushMappedNamedBufferRange(m_handle, offset, size);
}

void Buffer::bindAsAtomicCounter(uint32_t binding) const {
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, m_handle);
}

void Buffer::bindAsTransformFeedback(uint32_t binding) const {
    glBindBufferBase(GL_TRANSFORM_FEEDBACK, binding, m_handle);
}

void Buffer::bindAsUniform(uint32_t binding) const {
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, m_handle);
}

void Buffer::bindAsShaderStorage(uint32_t binding) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, m_handle);
}

void Buffer::unbindAsAtomicCounter(uint32_t binding) {
    glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, binding, 0);
}

void Buffer::unbindAsTransformFeedback(uint32_t binding) {
    glBindBufferBase(GL_TRANSFORM_FEEDBACK, binding, 0);
}

void Buffer::unbindAsUniform(uint32_t binding) {
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, 0);
}

void Buffer::unbindAsShaderStorage(uint32_t binding) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, 0);
}

void Buffer::bindRangeAsAtomicCounter(uint32_t binding, intptr_t offset, size_t size) const {
    glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, binding, m_handle, GLintptr(offset), GLsizeiptr(size));
}

void Buffer::bindRangeAsTransformFeedback(uint32_t binding, intptr_t offset, size_t size) const {
    glBindBufferRange(GL_TRANSFORM_FEEDBACK, binding, m_handle, GLintptr(offset), GLsizeiptr(size));
}

void Buffer::bindRangeAsUniform(uint32_t binding, intptr_t offset, size_t size) const {
    glBindBufferRange(GL_UNIFORM_BUFFER, binding, m_handle, GLintptr(offset), GLsizeiptr(size));
}

void Buffer::bindRangeAsShaderStorage(uint32_t binding, intptr_t offset, size_t size) const {
    glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, m_handle, GLintptr(offset), GLsizeiptr(size));
}
} // namespace gfx