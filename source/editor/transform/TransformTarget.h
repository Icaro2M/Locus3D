/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/command/transform/NodeTransformSnapshot.h"
#include "editor/scene/SceneNodeId.h"

namespace locus::editor {

    class EditorScene;
    class SceneNode;

    /**
     * @brief Captures transform data for one node participating in a transform session.
     *
     * A target stores the initial transform, the current preview transform, and the
     * node identifier. It does not own the scene node.
     */
    class TransformTarget {
    public:
        /**
         * @brief Captures a transform target from a scene node.
         *
         * @param node Node to capture.
         * @return Captured transform target.
         */
        [[nodiscard]] static TransformTarget capture(const SceneNode& node);

        /**
         * @brief Returns the target node identifier.
         *
         * @return Node identifier.
         */
        [[nodiscard]] SceneNodeId node() const;

        /**
         * @brief Returns the transform captured when the session started.
         *
         * @return Initial transform snapshot.
         */
        [[nodiscard]] const NodeTransformSnapshot& initial_transform() const;

        /**
         * @brief Returns the current preview transform.
         *
         * @return Preview transform snapshot.
         */
        [[nodiscard]] const NodeTransformSnapshot& preview_transform() const;

        /**
         * @brief Changes the current preview transform.
         *
         * @param transform New preview transform.
         */
        void set_preview_transform(const NodeTransform& transform);

        /**
         * @brief Restores the preview transform to the initially captured transform.
         */
        void reset_preview();

        /**
         * @brief Applies the current preview transform to the scene node.
         *
         * @param scene Scene that owns the target node.
         * @return True when the node exists and was updated.
         */
        bool apply_preview(EditorScene& scene) const;

        /**
         * @brief Restores the initially captured transform to the scene node.
         *
         * @param scene Scene that owns the target node.
         * @return True when the node exists and was updated.
         */
        bool restore(EditorScene& scene) const;

        /**
         * @brief Checks whether the preview transform differs from the initial one.
         *
         * @param epsilon Floating-point tolerance.
         * @return True when the preview changed.
         */
        [[nodiscard]] bool has_transform_change(float epsilon = 0.00001f) const;

    private:
        SceneNodeId node_{};
        NodeTransformSnapshot initialTransform_{};
        NodeTransformSnapshot previewTransform_{};
    };

} // namespace locus::editor