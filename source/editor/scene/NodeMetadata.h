/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

namespace locus::editor {

    /**
     * @brief User-facing and editor-facing metadata stored by scene nodes.
     */
    struct NodeMetadata {
        /**
         * @brief Human-readable node name.
         */
        std::string name;

        /**
         * @brief True when the node should be visible in editor views.
         */
        bool visible = true;

        /**
         * @brief True when editing operations should not modify this node.
         */
        bool locked = false;

        /**
         * @brief True when picking and selection may target this node.
         */
        bool selectable = true;

        /**
         * @brief True when the node should appear expanded in hierarchy views.
         */
        bool expanded = true;
    };

}