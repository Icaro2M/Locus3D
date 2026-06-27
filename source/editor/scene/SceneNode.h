/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/EditorTypes.h"
#include "editor/scene/NodeMetadata.h"
#include "editor/scene/NodePivot.h"
#include "editor/scene/NodeTransform.h"
#include "editor/scene/NodeType.h"
#include "editor/scene/SceneNodeId.h"

#include <vector>

namespace locus::editor {

    /**
     * @brief Base object stored by the editor scene hierarchy.
     */
    class SceneNode {
    public:
        /**
         * @brief Destroys the scene node.
         */
        virtual ~SceneNode() = default;

        SceneNode(const SceneNode&) = delete;
        SceneNode& operator=(const SceneNode&) = delete;

        /**
         * @brief Returns the stable node identifier.
         *
         * @return Node identifier.
         */
        [[nodiscard]] SceneNodeId id() const;

        /**
         * @brief Returns the parent node identifier.
         *
         * @return Parent identifier, or invalid when this node is a root.
         */
        [[nodiscard]] SceneNodeId parent() const;

        /**
         * @brief Returns the node runtime type.
         *
         * @return Node type.
         */
        [[nodiscard]] NodeType type() const;

        /**
         * @brief Returns the local transform.
         *
         * @return Mutable transform reference.
         */
        [[nodiscard]] NodeTransform& transform();

        /**
         * @brief Returns the local transform.
         *
         * @return Read-only transform reference.
         */
        [[nodiscard]] const NodeTransform& transform() const;

        /**
         * @brief Returns pivot information.
         *
         * @return Mutable pivot reference.
         */
        [[nodiscard]] NodePivot& pivot();

        /**
         * @brief Returns pivot information.
         *
         * @return Read-only pivot reference.
         */
        [[nodiscard]] const NodePivot& pivot() const;

        /**
         * @brief Returns node metadata.
         *
         * @return Mutable metadata reference.
         */
        [[nodiscard]] NodeMetadata& metadata();

        /**
         * @brief Returns node metadata.
         *
         * @return Read-only metadata reference.
         */
        [[nodiscard]] const NodeMetadata& metadata() const;

        /**
         * @brief Returns the direct children of this node.
         *
         * @return Ordered child identifier list.
         */
        [[nodiscard]] const std::vector<SceneNodeId>& children() const;

        /**
         * @brief Checks whether this node has a valid parent.
         *
         * @return True when the node is parented.
         */
        [[nodiscard]] bool has_parent() const;

        /**
         * @brief Checks whether this node should be visible.
         *
         * @return True when the node metadata marks it as visible.
         */
        [[nodiscard]] bool is_visible() const;

        /**
         * @brief Checks whether this node can be selected.
         *
         * @return True when the node is visible, selectable, and not locked.
         */
        [[nodiscard]] bool is_selectable() const;

        /**
         * @brief Marks this node as dirty.
         *
         * @param flags Dirty flags to add.
         */
        void mark_dirty(EditorDirtyFlags flags);

        /**
         * @brief Clears dirty flags from this node.
         *
         * @param flags Dirty flags to clear.
         */
        void clear_dirty(EditorDirtyFlags flags = EditorDirtyFlags::All);

        /**
         * @brief Returns the current dirty flag mask.
         *
         * @return Dirty flags.
         */
        [[nodiscard]] EditorDirtyFlags dirty_flags() const;

    protected:
        /**
         * @brief Creates a scene node.
         *
         * @param id Stable node identifier.
         * @param type Runtime node type.
         * @param name Human-readable node name.
         */
        SceneNode(SceneNodeId id, NodeType type, std::string name);

    private:
        friend class SceneTree;

        void set_parent(SceneNodeId parent);
        void add_child(SceneNodeId child);
        void remove_child(SceneNodeId child);

        SceneNodeId id_{};
        SceneNodeId parent_{};
        NodeType type_ = NodeType::Empty;
        NodeTransform transform_{};
        NodePivot pivot_{};
        NodeMetadata metadata_{};
        std::vector<SceneNodeId> children_{};
        EditorDirtyFlags dirtyFlags_ = EditorDirtyFlags::All;
    };

}