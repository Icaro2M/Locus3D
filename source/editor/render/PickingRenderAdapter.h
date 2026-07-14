/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstddef>
#include <string>

namespace locus::graphics {

    struct RenderObject;
    class RenderScene;

} // namespace locus::graphics

namespace locus::editor {

    class PickingSync;

    /**
     * @brief Diagnostics produced while applying picking IDs to render objects.
     */
    struct PickingRenderResult {
        /**
         * @brief Number of render objects inspected.
         */
        std::size_t visitedObjectCount = 0;

        /**
         * @brief Number of objects that received a valid picking ID.
         */
        std::size_t assignedObjectCount = 0;

        /**
         * @brief Number of objects cleared because no mapping was found.
         */
        std::size_t unmappedObjectCount = 0;

        /**
         * @brief Number of objects ignored because their stable ID was zero.
         */
        std::size_t invalidObjectCount = 0;

        /**
         * @brief Human-readable operation diagnostic.
         */
        std::string message;

        /**
         * @brief Checks whether at least one picking ID was assigned.
         *
         * @return True when one or more render objects became pickable.
         */
        [[nodiscard]] bool has_assignments() const {
            return assignedObjectCount > 0;
        }
    };

    /**
     * @brief Applies editor picking mappings to graphics render objects.
     *
     * PickingRenderAdapter resolves each RenderObject stable identifier as a
     * SceneNodeId and assigns the corresponding compact PickingId maintained by
     * PickingSync.
     *
     * The adapter does not create mappings, render picking geometry, access
     * OpenGL, or alter stable RenderObject identifiers.
     */
    class PickingRenderAdapter {
    public:
        /**
         * @brief Applies a picking mapping to one render object.
         *
         * Objects with a zero stable ID or without a matching PickingSync entry
         * receive an invalid picking ID.
         *
         * @param object Render object to update.
         * @param sync Synchronized editor-to-picking mapping table.
         * @return True when a valid picking ID was assigned.
         */
        static bool apply_to_object(
            graphics::RenderObject& object,
            const PickingSync& sync
        );

        /**
         * @brief Applies picking mappings to every object in a render scene.
         *
         * @param scene Render scene to update.
         * @param sync Synchronized editor-to-picking mapping table.
         * @param result Optional diagnostic output.
         */
        static void apply_to_scene(
            graphics::RenderScene& scene,
            const PickingSync& sync,
            PickingRenderResult* result = nullptr
        );
    };

} // namespace locus::editor