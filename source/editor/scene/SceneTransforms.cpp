/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/scene/SceneTransforms.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNode.h"

namespace locus::editor {

    glm::mat4 SceneTransforms::world_matrix(
        const EditorScene& scene,
        SceneNodeId node)
    {
        const SceneNode* sceneNode = scene.find_node(node);
        if (sceneNode == nullptr) {
            return glm::mat4{ 1.0f };
        }

        const glm::mat4 local = sceneNode->transform().matrix();
        if (sceneNode->parent().is_invalid()) {
            return local;
        }

        return world_matrix(scene, sceneNode->parent()) * local;
    }

} // namespace locus::editor
