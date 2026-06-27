/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief Runtime category of an editor scene node.
     */
    enum class NodeType {
        /**
         * @brief Empty transform-only node.
         */
        Empty,

        /**
         * @brief Editable polygon mesh node backed by a LEM.
         */
        Mesh
    };

}