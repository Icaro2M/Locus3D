#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/Framebuffer.h"
#include "graphics/picking/PickingId.h"

namespace locus::graphics
{
    class PickingBuffer
    {
    public:
        PickingBuffer() = default;
        ~PickingBuffer();

        PickingBuffer(const PickingBuffer&) = delete;
        PickingBuffer& operator=(const PickingBuffer&) = delete;

        PickingBuffer(PickingBuffer&& other) noexcept;
        PickingBuffer& operator=(PickingBuffer&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(i32 width, i32 height);
        [[nodiscard]] GraphicsResult<void> resize(i32 width, i32 height);

        void destroy();

        void bind() const;
        static void bind_default();

        void clear() const;

        [[nodiscard]] PickingId read_id(i32 x, i32 y) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] i32 width() const;
        [[nodiscard]] i32 height() const;

        [[nodiscard]] const Framebuffer& framebuffer() const;

    private:
        Framebuffer framebuffer_;
    };
}