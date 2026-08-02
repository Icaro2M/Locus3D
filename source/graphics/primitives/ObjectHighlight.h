/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"
#include "graphics/scene/RenderObject.h"

#include <vector>

namespace locus::graphics
{
    /**
     * @brief Object-level highlight category consumed by viewport outline passes.
     */
    enum class ObjectHighlightCategory
    {
        Hovered,
        Selected
    };

    /**
     * @brief Generic graphics submission for one object outline candidate.
     */
    struct ObjectHighlight
    {
        /**
         * @brief Render object to draw into the highlight mask.
         */
        const RenderObject* object = nullptr;

        /**
         * @brief Per-submission mask identifier, zero reserved for background.
         */
        u32 maskId = 0;

        /**
         * @brief Visual highlight category.
         */
        ObjectHighlightCategory category = ObjectHighlightCategory::Hovered;

        /**
         * @brief Checks whether this submission can be drawn.
         *
         * @return True when the referenced object and mask identifier are valid.
         */
        [[nodiscard]] bool is_valid() const noexcept
        {
            return object != nullptr && maskId != 0;
        }
    };

    /**
     * @brief Contiguous object highlight submission list.
     */
    struct ObjectHighlightBatch
    {
        std::vector<ObjectHighlight> highlights;

        [[nodiscard]] bool empty() const noexcept
        {
            return highlights.empty();
        }

        [[nodiscard]] std::size_t size() const noexcept
        {
            return highlights.size();
        }

        void clear()
        {
            highlights.clear();
        }
    };
}
