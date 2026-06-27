/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace locus::editor {

    /**
     * @brief High-level interaction mode currently used by the editor.
     */
    enum class EditorMode {
        /**
         * @brief Object-level editing and selection.
         */
        Object,

        /**
         * @brief Mesh component editing.
         */
        Mesh
    };

    /**
     * @brief Dirty flags used to mark editor subsystems that need synchronization.
     */
    enum class EditorDirtyFlags : std::uint32_t {
        /**
         * @brief No editor subsystem is dirty.
         */
        None = 0u,

        /**
         * @brief Scene hierarchy, object data, or object transforms changed.
         */
        Scene = 1u << 0u,

        /**
         * @brief Object or component selection changed.
         */
        Selection = 1u << 1u,

        /**
         * @brief Mesh topology, geometry, or attributes changed.
         */
        Mesh = 1u << 2u,

        /**
         * @brief Render-side representation needs to be rebuilt or updated.
         */
        Render = 1u << 3u,

        /**
         * @brief Picking representation needs to be rebuilt or updated.
         */
        Picking = 1u << 4u,

        /**
         * @brief Manufacturing analysis data needs to be recomputed.
         */
        Manufacturing = 1u << 5u,

        /**
         * @brief Every editor subsystem should be considered dirty.
         */
        All = Scene | Selection | Mesh | Render | Picking | Manufacturing
    };

    /**
     * @brief Combines two dirty flag masks.
     *
     * @param lhs Left-hand mask.
     * @param rhs Right-hand mask.
     * @return Combined mask.
     */
    [[nodiscard]] constexpr EditorDirtyFlags operator|(
        EditorDirtyFlags lhs,
        EditorDirtyFlags rhs)
    {
        return static_cast<EditorDirtyFlags>(
            static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Intersects two dirty flag masks.
     *
     * @param lhs Left-hand mask.
     * @param rhs Right-hand mask.
     * @return Intersected mask.
     */
    [[nodiscard]] constexpr EditorDirtyFlags operator&(
        EditorDirtyFlags lhs,
        EditorDirtyFlags rhs)
    {
        return static_cast<EditorDirtyFlags>(
            static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs));
    }

    /**
     * @brief Adds dirty flags to an existing mask.
     *
     * @param lhs Mask to update.
     * @param rhs Flags to add.
     * @return Updated mask.
     */
    constexpr EditorDirtyFlags& operator|=(
        EditorDirtyFlags& lhs,
        EditorDirtyFlags rhs)
    {
        lhs = lhs | rhs;
        return lhs;
    }

    /**
     * @brief Removes dirty flags from an existing mask.
     *
     * @param lhs Mask to update.
     * @param rhs Flags to remove.
     * @return Updated mask.
     */
    constexpr EditorDirtyFlags& operator&=(
        EditorDirtyFlags& lhs,
        EditorDirtyFlags rhs)
    {
        lhs = lhs & rhs;
        return lhs;
    }

    /**
     * @brief Checks whether a dirty flag mask contains a given flag.
     *
     * @param mask Mask to inspect.
     * @param flag Flag to test.
     * @return True when the flag is present.
     */
    [[nodiscard]] constexpr bool has_flag(EditorDirtyFlags mask, EditorDirtyFlags flag)
    {
        return (static_cast<std::uint32_t>(mask & flag) != 0u);
    }

}