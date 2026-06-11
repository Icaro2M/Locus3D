#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"

#include <cstddef>

namespace locus::graphics
{
    class UniformBuffer
    {
    public:
        UniformBuffer() = default;
        ~UniformBuffer();

        UniformBuffer(const UniformBuffer&) = delete;
        UniformBuffer& operator=(const UniformBuffer&) = delete;

        UniformBuffer(UniformBuffer&& other) noexcept;
        UniformBuffer& operator=(UniformBuffer&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(std::size_t size, BufferUsage usage = BufferUsage::Dynamic);
        void destroy();

        void bind() const;
        static void unbind();

        [[nodiscard]] GraphicsResult<void> resize(std::size_t size);
        [[nodiscard]] GraphicsResult<void> set_data(const void* data, std::size_t size, std::size_t offset = 0);

        [[nodiscard]] GraphicsResult<void> bind_base(u32 bindingPoint) const;
        [[nodiscard]] GraphicsResult<void> bind_range(u32 bindingPoint, std::size_t offset, std::size_t size) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] u32 id() const;
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] BufferUsage usage() const;

        [[nodiscard]] static u32 uniform_buffer_offset_alignment();

    private:
        [[nodiscard]] GraphicsResult<void> validate_binding_point(u32 bindingPoint) const;
        [[nodiscard]] GraphicsResult<void> validate_range(std::size_t offset, std::size_t size) const;

    private:
        u32 id_ = 0;
        std::size_t size_ = 0;
        BufferUsage usage_ = BufferUsage::Dynamic;
    };
}