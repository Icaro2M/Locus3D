/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{
    /**
     * @brief Object identifier encoded into the picking render target.
     */
    struct PickingId
    {
        /**
         * @brief Numeric ID value. Zero is reserved for "no hit".
         */
        u32 value = 0;

        /**
         * @brief Checks whether this ID can represent a selectable object.
         *
         * @return True when the value is non-zero.
         */
        [[nodiscard]] bool is_valid() const
        {
            return value != 0;
        }

        /**
         * @brief Returns the reserved invalid picking ID.
         *
         * @return Invalid picking ID.
         */
        [[nodiscard]] static PickingId invalid()
        {
            return PickingId{};
        }

        /**
         * @brief Creates a picking ID from an integer value.
         *
         * @param id Raw ID value.
         * @return Picking ID wrapper.
         */
        [[nodiscard]] static PickingId from_u32(u32 id)
        {
            return PickingId{ id };
        }
    };

    /**
     * @brief Compares two picking IDs for equality.
     *
     * @param a First ID.
     * @param b Second ID.
     * @return True when both raw values match.
     */
    [[nodiscard]] inline bool operator==(PickingId a, PickingId b)
    {
        return a.value == b.value;
    }

    /**
     * @brief Compares two picking IDs for inequality.
     *
     * @param a First ID.
     * @param b Second ID.
     * @return True when the raw values differ.
     */
    [[nodiscard]] inline bool operator!=(PickingId a, PickingId b)
    {
        return !(a == b);
    }

    /**
     * @brief Encodes a 24-bit picking ID into an RGB color.
     *
     * @param id Picking ID to encode.
     * @return RGBA color storing the lower 24 bits of the ID.
     */
    [[nodiscard]] inline ColorRGBA encode_picking_id(PickingId id)
    {
        const u32 value = id.value & 0x00FFFFFFu;

        const float r = static_cast<float>((value >> 16u) & 0xFFu) / 255.0f;
        const float g = static_cast<float>((value >> 8u) & 0xFFu) / 255.0f;
        const float b = static_cast<float>(value & 0xFFu) / 255.0f;

        return ColorRGBA{ r, g, b, 1.0f };
    }

    /**
     * @brief Decodes a picking ID from RGB byte channels.
     *
     * @param r Red channel byte.
     * @param g Green channel byte.
     * @param b Blue channel byte.
     * @return Decoded picking ID.
     */
    [[nodiscard]] inline PickingId decode_picking_id(u8 r, u8 g, u8 b)
    {
        const u32 value =
            (static_cast<u32>(r) << 16u) |
            (static_cast<u32>(g) << 8u) |
            static_cast<u32>(b);

        return PickingId{ value };
    }
}
