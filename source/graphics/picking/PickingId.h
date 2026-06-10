#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    struct PickingId
    {
        u32 value = 0;

        [[nodiscard]] bool is_valid() const
        {
            return value != 0;
        }

        [[nodiscard]] static PickingId invalid()
        {
            return PickingId{};
        }

        [[nodiscard]] static PickingId from_u32(u32 id)
        {
            return PickingId{ id };
        }
    };

    [[nodiscard]] inline bool operator==(PickingId a, PickingId b)
    {
        return a.value == b.value;
    }

    [[nodiscard]] inline bool operator!=(PickingId a, PickingId b)
    {
        return !(a == b);
    }

    [[nodiscard]] inline ColorRGBA encode_picking_id(PickingId id)
    {
        const u32 value = id.value & 0x00FFFFFFu;

        const float r = static_cast<float>((value >> 16u) & 0xFFu) / 255.0f;
        const float g = static_cast<float>((value >> 8u) & 0xFFu) / 255.0f;
        const float b = static_cast<float>(value & 0xFFu) / 255.0f;

        return ColorRGBA{ r, g, b, 1.0f };
    }

    [[nodiscard]] inline PickingId decode_picking_id(u8 r, u8 g, u8 b)
    {
        const u32 value =
            (static_cast<u32>(r) << 16u) |
            (static_cast<u32>(g) << 8u) |
            static_cast<u32>(b);

        return PickingId{ value };
    }
}