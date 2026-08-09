/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"

#include <vector>

#include <glm/vec3.hpp>

namespace locus::editor {

    class EditorScene;

    /**
     * @brief Pivot policy used by transform sessions.
     */
    enum class TransformPivotMode {
        /**
         * @brief Uses the average world pivot of all selected targets.
         */
        SelectionCenter,

        /**
         * @brief Uses the active object's world pivot when available.
         */
        ActiveObject,

        /**
         * @brief Uses each target own origin during rotation and scale.
         */
        IndividualOrigins,

        /**
         * @brief Uses the world origin.
         */
        WorldOrigin,

        /**
         * @brief Uses a caller-provided world-space pivot.
         */
        Custom
    };

    /**
     * @brief Resolves world-space pivot positions for transform sessions.
     */
    class TransformPivotResolver {
    public:
        /**
         * @brief Resolves a shared world-space pivot.
         *
         * @param scene Scene that owns the selected nodes.
         * @param targets Nodes participating in the session.
         * @param active Active object, or invalid when none is active.
         * @param mode Pivot mode.
         * @param customPivot Custom pivot used when mode is Custom.
         * @return Resolved world-space pivot.
         */
        [[nodiscard]] static glm::vec3 resolve(
            const EditorScene& scene,
            const std::vector<SceneNodeId>& targets,
            SceneNodeId active,
            TransformPivotMode mode,
            const glm::vec3& customPivot = glm::vec3{ 0.0f, 0.0f, 0.0f });

        /**
         * @brief Computes the world-space pivot of a node.
         *
         * @param scene Scene that owns the node.
         * @param node Node identifier.
         * @return Node pivot in world coordinates, or world origin when missing.
         */
        [[nodiscard]] static glm::vec3 node_pivot_position(
            const EditorScene& scene,
            SceneNodeId node);

        /**
         * @brief Computes the world-space origin of a node transform.
         *
         * @param scene Scene that owns the node.
         * @param node Node identifier.
         * @return Node origin in world coordinates, or world origin when missing.
         */
        [[nodiscard]] static glm::vec3 node_origin_position(
            const EditorScene& scene,
            SceneNodeId node);

        /**
         * @brief Converts a world-space pivot position into node-local offset.
         *
         * @param scene Scene that owns the node.
         * @param node Node identifier.
         * @param worldPosition Desired pivot position in world coordinates.
         * @return Node-local pivot offset, or zero when the node is missing.
         */
        [[nodiscard]] static glm::vec3 node_local_offset_from_world(
            const EditorScene& scene,
            SceneNodeId node,
            const glm::vec3& worldPosition);
    };

} // namespace locus::editor
