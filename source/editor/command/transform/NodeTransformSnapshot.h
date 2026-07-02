/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/NodeTransform.h"

namespace locus::editor {

    /**
     * @brief Value snapshot of a scene node local transform.
     *
     * This type is intentionally lightweight and independent from SceneNode so
     * transform commands can store undo/redo state without owning scene objects.
     */
    struct NodeTransformSnapshot {
        /**
         * @brief Captured local transform.
         */
        NodeTransform transform{};

        /**
         * @brief Captures a transform snapshot.
         *
         * @param value Transform to capture.
         * @return Captured snapshot.
         */
        [[nodiscard]] static NodeTransformSnapshot capture(const NodeTransform& value) {
            NodeTransformSnapshot snapshot{};
            snapshot.transform = value;
            return snapshot;
        }

        /**
         * @brief Applies the snapshot to a transform.
         *
         * @param value Transform to overwrite.
         */
        void apply_to(NodeTransform& value) const {
            value = transform;
        }
    };

}