/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace locus::editor {

    /**
     * @brief Bit mask describing which snapping families are enabled.
     */
    enum class SnapMode : std::uint32_t {
        /**
         * @brief No snapping mode is enabled.
         */
        None = 0u,

        /**
         * @brief Snaps positions to the editor grid.
         */
        Grid = 1u << 0u,

        /**
         * @brief Snaps positions to mesh vertices.
         */
        Vertex = 1u << 1u,

        /**
         * @brief Snaps positions to mesh edges.
         */
        Edge = 1u << 2u,

        /**
         * @brief Snaps positions to mesh faces.
         */
        Face = 1u << 3u,

        /**
         * @brief Snaps movement deltas to a linear increment.
         */
        Increment = 1u << 4u,

        /**
         * @brief Snaps angular movement to an angle increment.
         */
        Angle = 1u << 5u,

        /**
         * @brief Enables every snapping family.
         */
        All = (1u << 0u) | (1u << 1u) | (1u << 2u) | (1u << 3u) | (1u << 4u) | (1u << 5u)
    };

    /**
     * @brief Combines two snap mode masks.
     *
     * @param lhs Left-hand mask.
     * @param rhs Right-hand mask.
     * @return Combined mask.
     */
    [[nodiscard]] constexpr SnapMode operator|(SnapMode lhs, SnapMode rhs)
    {
        return static_cast<SnapMode>(
            static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Intersects two snap mode masks.
     *
     * @param lhs Left-hand mask.
     * @param rhs Right-hand mask.
     * @return Intersected mask.
     */
    [[nodiscard]] constexpr SnapMode operator&(SnapMode lhs, SnapMode rhs)
    {
        return static_cast<SnapMode>(
            static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Adds snap modes to an existing mask.
     *
     * @param lhs Mask to update.
     * @param rhs Modes to add.
     * @return Updated mask.
     */
    constexpr SnapMode& operator|=(SnapMode& lhs, SnapMode rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    /**
     * @brief Keeps only modes present in both masks.
     *
     * @param lhs Mask to update.
     * @param rhs Modes to keep.
     * @return Updated mask.
     */
    constexpr SnapMode& operator&=(SnapMode& lhs, SnapMode rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    /**
     * @brief Checks whether a snap mode mask contains a mode.
     *
     * @param mask Mask to inspect.
     * @param mode Mode to test.
     * @return True when the mode is present.
     */
    [[nodiscard]] constexpr bool has_snap_mode(SnapMode mask, SnapMode mode)
    {
        return static_cast<std::uint32_t>(mask & mode) != 0u;
    }

} // namespace locus::editor