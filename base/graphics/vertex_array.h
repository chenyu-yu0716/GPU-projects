#pragma once

#include <cstddef>
#include <cstdint>

#include "graphics/buffer.h"
#include "graphics/rhi_resource.h"

namespace gfx {
class VertexArray : public RHIResource {
public:
    enum class DataType { Int8, UInt8, Int16, UInt16, Int32, UInt32, Float16, Float32, Float64 };

public:
    VertexArray();

    VertexArray(VertexArray&& rhs) noexcept;

    ~VertexArray();

    VertexArray& operator=(VertexArray&& rhs) noexcept;

    void bind() const;

    static void unbind();

    // Non-DSA API
    static void enableVertexAttribArray(uint32_t location);

    static void disableVertexAttribArray(uint32_t location);

    static void vertexAttribPointer(
        uint32_t location, size_t size, DataType type, bool normalized, size_t stride, void const* pointer = nullptr);

    static void vertexAttribDivisor(uint32_t location, uint32_t divisor);

    // DSA API
    void attachVertexBuffer(Buffer const& buffer, uint32_t index, intptr_t offset, size_t stride);

    void attachIndexBuffer(Buffer const& buffer);

    void enableAttrib(uint32_t location);

    void disableAttrib(uint32_t location);

    void setAttribFormat(uint32_t location, size_t size, DataType type, bool normalized, size_t relativeOffset);

    void setAttribIFormat(uint32_t location, size_t size, DataType type, size_t relativeOffset);

    void setAttribLFormat(uint32_t location, size_t size, size_t relativeOffset);

    void setAttribBinding(uint32_t location, uint32_t index);

    void setBindingDivisor(uint32_t index, uint32_t divisor);
};
} // namespace gfx
