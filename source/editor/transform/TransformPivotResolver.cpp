/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/transform/TransformPivotResolver.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace locus::editor {

    namespace {

        [[nodiscard]] glm::mat4 node_world_matrix(const EditorScene& scene, SceneNodeId node)
        {
            const SceneNode* sceneNode = scene.find_node(node);
            if (!sceneNode) {
                return glm::mat4{ 1.0f };
            }

            const glm::mat4 localMatrix = sceneNode->transform().matrix();

            if (sceneNode->parent().is_invalid()) {
                return localMatrix;
            }

            return node_world_matrix(scene, sceneNode->parent()) * localMatrix;
        }

        [[nodiscard]] glm::vec3 matrix_translation(const glm::mat4& matrix)
        {
            return glm::vec3{ matrix[3] };
        }

    } // namespace

    glm::vec3 TransformPivotResolver::resolve(
        const EditorScene& scene,
        const std::vector<SceneNodeId>& targets,
        SceneNodeId active,
        TransformPivotMode mode,
        const glm::vec3& customPivot)
    {
        if (mode == TransformPivotMode::Custom) {
            return customPivot;
        }

        if (mode == TransformPivotMode::WorldOrigin) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        if (mode == TransformPivotMode::ActiveObject && active.is_valid()) {
            if (scene.find_node(active)) {
                return node_pivot_position(scene, active);
            }
        }

        if (targets.empty()) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        glm::vec3 sum{ 0.0f, 0.0f, 0.0f };
        std::size_t count = 0;

        for (SceneNodeId target : targets) {
            if (!scene.find_node(target)) {
                continue;
            }

            sum += node_pivot_position(scene, target);
            ++count;
        }

        if (count == 0) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        return sum / static_cast<float>(count);
    }

    glm::vec3 TransformPivotResolver::node_pivot_position(
        const EditorScene& scene,
        SceneNodeId node)
    {
        const SceneNode* sceneNode = scene.find_node(node);
        if (!sceneNode) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        const glm::mat4 worldMatrix = node_world_matrix(scene, node);

        if (!sceneNode->pivot().custom) {
            return matrix_translation(worldMatrix);
        }

        return glm::vec3{ worldMatrix * glm::vec4{ sceneNode->pivot().offset, 1.0f } };
    }

    glm::vec3 TransformPivotResolver::node_origin_position(
        const EditorScene& scene,
        SceneNodeId node)
    {
        return matrix_translation(node_world_matrix(scene, node));
    }

} // namespace locus::editor