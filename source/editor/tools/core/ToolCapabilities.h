/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace locus::editor {

    /**
     * @brief Capability flags describing editor tool requirements and behavior.
     */
    enum class ToolCapabilities : std::uint32_t {
        /**
         * @brief Tool has no special capability or requirement.
         */
        None = 0u,

        /**
         * @brief Tool supports object-level interaction.
         */
        ObjectMode = 1u << 0u,

        /**
         * @brief Tool supports mesh component interaction.
         */
        MeshMode = 1u << 1u,

        /**
         * @brief Tool requires at least one valid selection target.
         */
        RequiresSelection = 1u << 2u,

        /**
         * @brief Tool uses pointer input.
         */
        UsesPointer = 1u << 3u,

        /**
         * @brief Tool uses transform gizmo interaction.
         */
        UsesGizmo = 1u << 4u,

        /**
         * @brief Tool can consume editor snapping services.
         */
        UsesSnapping = 1u << 5u,

        /**
         * @brief Tool produces temporary preview geometry or state.
         */
        UsesPreview = 1u << 6u,

        /**
         * @brief Tool owns a confirmable and cancellable interaction session.
         */
        Modal = 1u << 7u
    };

    /**
     * @brief Combines two tool capability masks.
     *
     * @param lhs Left-hand mask.
     * @param rhs Right-hand mask.
     * @return Combined capability mask.
     */
    [[nodiscard]] constexpr ToolCapabilities operator|(
        ToolCapabilities lhs,
        ToolCapabilities rhs) {

        return static_cast<ToolCapabilities>(
            static_cast<std::uint32_t>(lhs) |
            static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Intersects two tool capability masks.
     *
     * @param lhs Left-hand mask.
     * @param rhs Right-hand mask.
     * @return Intersected capability mask.
     */
    [[nodiscard]] constexpr ToolCapabilities operator&(
        ToolCapabilities lhs,
        ToolCapabilities rhs) {

        return static_cast<ToolCapabilities>(
            static_cast<std::uint32_t>(lhs) &
            static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Adds capabilities to an existing mask.
     *
     * @param lhs Mask to update.
     * @param rhs Capabilities to add.
     * @return Updated capability mask.
     */
    constexpr ToolCapabilities& operator|=(
        ToolCapabilities& lhs,
        ToolCapabilities rhs) {

        lhs = lhs | rhs;
        return lhs;
    }

    /**
     * @brief Checks whether a capability mask contains a given capability.
     *
     * @param mask Mask to inspect.
     * @param capability Capability to test.
     * @return True when the capability is present.
     */
    [[nodiscard]] constexpr bool has_capability(
        ToolCapabilities mask,
        ToolCapabilities capability) {

        return (
            static_cast<std::uint32_t>(mask & capability) != 0u);
    }

} // namespace locus::editor