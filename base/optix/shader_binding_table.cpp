#include <optix/shader_binding_table.h>

#include <gpu/utility.h>

#include <cstdint>
#include <stdexcept>
#include <utility>

namespace optix {

ShaderBindingTable::ShaderBindingTable(ShaderBindingTable&& other) noexcept
    : m_table(std::exchange(other.m_table, OptixShaderBindingTable{}))
    , m_raygen(std::move(other.m_raygen))
    , m_exception(std::move(other.m_exception))
    , m_miss(std::move(other.m_miss))
    , m_hitgroup(std::move(other.m_hitgroup))
    , m_callables(std::move(other.m_callables)) {}

ShaderBindingTable& ShaderBindingTable::operator=(ShaderBindingTable&& other) noexcept {
    if (this != &other) {
        m_raygen = std::move(other.m_raygen);
        m_exception = std::move(other.m_exception);
        m_miss = std::move(other.m_miss);
        m_hitgroup = std::move(other.m_hitgroup);
        m_callables = std::move(other.m_callables);
        m_table = std::exchange(other.m_table, OptixShaderBindingTable{});
    }

    return *this;
}

void ShaderBindingTable::upload(gpu::Buffer<std::byte>& target, void const* records, size_t stride, uint32_t count) {
    if (count == 0) {
        target = gpu::Buffer<std::byte>{};
        return;
    }

    if (!records || stride < OPTIX_SBT_RECORD_HEADER_SIZE || stride % OPTIX_SBT_RECORD_ALIGNMENT) {
        throw std::invalid_argument("Invalid SBT records or stride");
    }

    if (stride > UINT32_MAX || count > SIZE_MAX / stride) {
        throw std::length_error("SBT size overflow");
    }

    gpu::Buffer<std::byte> uploaded(stride * count);
    CHECK_CUDA(cudaMemcpy(uploaded.data(), records, stride * count, cudaMemcpyHostToDevice));
    target = std::move(uploaded);
}

void ShaderBindingTable::setRaygen(void const* records, size_t stride) {
    upload(m_raygen, records, stride, 1);
    m_table.raygenRecord = address(m_raygen.data());
}

void ShaderBindingTable::setException(void const* records, size_t stride) {
    upload(m_exception, records, stride, 1);
    m_table.exceptionRecord = address(m_exception.data());
}

void ShaderBindingTable::setMiss(void const* records, size_t stride, uint32_t count) {
    upload(m_miss, records, stride, count);
    m_table.missRecordBase = address(m_miss.data());
    m_table.missRecordStrideInBytes = count ? static_cast<uint32_t>(stride) : 0;
    m_table.missRecordCount = count;
}

void ShaderBindingTable::setHitgroup(void const* records, size_t stride, uint32_t count) {
    upload(m_hitgroup, records, stride, count);
    m_table.hitgroupRecordBase = address(m_hitgroup.data());
    m_table.hitgroupRecordStrideInBytes = count ? static_cast<uint32_t>(stride) : 0;
    m_table.hitgroupRecordCount = count;
}

void ShaderBindingTable::setCallables(void const* records, size_t stride, uint32_t count) {
    upload(m_callables, records, stride, count);
    m_table.callablesRecordBase = address(m_callables.data());
    m_table.callablesRecordStrideInBytes = count ? static_cast<uint32_t>(stride) : 0;
    m_table.callablesRecordCount = count;
}

} // namespace optix
