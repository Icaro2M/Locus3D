/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/transform/NodeTransformSnapshot.h"
#include "editor/scene/SceneNodeId.h"

namespace locus::editor {

    /**
     * @brief Absolute transform change for one editor scene node.
     *
     * The record stores both sides of an undoable transform operation. It is
     * independent from live scene nodes and may safely be owned by command
     * history.
     */
    struct NodeTransformChange {
        /**
         * @brief Scene node affected by the change.
         */
        SceneNodeId node{};

        /**
         * @brief Transform that existed before the operation.
         */
        NodeTransformSnapshot previous{};

        /**
         * @brief Transform produced by the operation.
         */
        NodeTransformSnapshot next{};

        /**
         * @brief Checks whether the record references a valid node.
         *
         * @return True when the node identifier is valid.
         */
        [[nodiscard]] bool is_valid() const {
            return node.is_valid();
        }
    };

} // namespace locus::editor