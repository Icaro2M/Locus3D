/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"

#include <glm/mat4x4.hpp>

namespace locus::editor {

    class EditorScene;

    /**
     * @brief Computes transform matrices for editor scene nodes.
     */
    class SceneTransforms {
    public:
        /**
         * @brief Builds the complete local-to-world matrix for a node.
         *
         * @param scene Scene containing the node hierarchy.
         * @param node Node whose transform should be resolved.
         * @return Local-to-world matrix, or identity when the node is missing.
         */
        [[nodiscard]] static glm::mat4 world_matrix(
            const EditorScene& scene,
            SceneNodeId node);
    };

} // namespace locus::editor
