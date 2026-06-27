/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNode.h"

namespace locus::editor {

    /**
     * @brief Transform-only scene node used for grouping and pivots.
     */
    class EmptyNode final : public SceneNode {
    public:
        /**
         * @brief Creates an empty scene node.
         *
         * @param id Stable node identifier.
         * @param name Human-readable node name.
         */
        EmptyNode(SceneNodeId id, const std::string& name)
            : SceneNode(id, NodeType::Empty, name)
        {
        }
    };

}