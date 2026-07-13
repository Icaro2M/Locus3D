/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "graphics/picking/PickingId.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace locus::editor {

    class EditorScene;

    /**
     * @brief Diagnostics produced while synchronizing picking identifiers.
     */
    struct PickingSyncResult {
        /**
         * @brief True when the picking table was synchronized.
         */
        bool synchronized = false;

        /**
         * @brief Number of scene nodes inspected.
         */
        std::size_t sceneNodeCount = 0;

        /**
         * @brief Number of mappings preserved from the previous state.
         */
        std::size_t preservedMappingCount = 0;

        /**
         * @brief Number of mappings created for new scene nodes.
         */
        std::size_t createdMappingCount = 0;

        /**
         * @brief Number of mappings removed for deleted scene nodes.
         */
        std::size_t removedMappingCount = 0;

        /**
         * @brief Number of active mappings after synchronization.
         */
        std::size_t mappingCount = 0;

        /**
         * @brief True when one or more nodes could not receive a picking ID.
         */
        bool exhausted = false;

        /**
         * @brief Human-readable synchronization diagnostic.
         */
        std::string message;
    };

    /**
     * @brief Maintains compact picking identifiers for editor scene nodes.
     *
     * PickingSync owns the bidirectional mapping between stable editor
     * SceneNodeId values and compact graphics PickingId values.
     *
     * Existing mappings are preserved while their scene nodes remain alive.
     * Removed mappings release their compact IDs for later reuse.
     *
     * The synchronizer does not modify RenderScene, RenderObject, EditorScene, or
     * GPU resources. Applying the generated identifiers to render objects remains
     * the responsibility of PickingRenderAdapter.
     */
    class PickingSync {
    public:
        /**
         * @brief Maximum picking ID representable by the RGB picking target.
         *
         * Zero is reserved for no hit, leaving values 1 through 0x00FFFFFF
         * available for selectable objects.
         */
        static constexpr graphics::u32 MaxPickingId = 0x00FFFFFFu;

        /**
         * @brief Creates an empty picking synchronizer.
         */
        PickingSync() = default;

        /**
         * @brief Removes every mapping and resets identifier allocation.
         */
        void clear();

        /**
         * @brief Synchronizes the mapping table with an editor scene.
         *
         * Existing nodes retain their current picking IDs. Deleted nodes are
         * removed and their IDs become available for reuse. New nodes receive
         * previously released IDs or newly allocated IDs.
         *
         * @param scene Source editor scene.
         * @return True when every scene node received a valid picking ID.
         */
        bool sync(const EditorScene& scene);

        /**
         * @brief Checks whether a scene node has an assigned picking ID.
         *
         * @param nodeId Scene node identifier.
         * @return True when the node has an active mapping.
         */
        [[nodiscard]] bool contains(SceneNodeId nodeId) const;

        /**
         * @brief Checks whether a compact picking ID has an assigned scene node.
         *
         * @param pickingId Compact picking identifier.
         * @return True when the picking ID has an active mapping.
         */
        [[nodiscard]] bool contains(
            graphics::PickingId pickingId
        ) const;

        /**
         * @brief Resolves a scene node into its compact picking ID.
         *
         * @param nodeId Scene node identifier.
         * @return Assigned picking ID, or invalid when no mapping exists.
         */
        [[nodiscard]] graphics::PickingId picking_id(
            SceneNodeId nodeId
        ) const;

        /**
         * @brief Resolves a compact picking ID into its editor scene node.
         *
         * @param pickingId Compact picking identifier.
         * @return Assigned scene node ID, or invalid when no mapping exists.
         */
        [[nodiscard]] SceneNodeId scene_node_id(
            graphics::PickingId pickingId
        ) const;

        /**
         * @brief Returns the number of active mappings.
         *
         * @return Active mapping count.
         */
        [[nodiscard]] std::size_t size() const;

        /**
         * @brief Checks whether the mapping table is empty.
         *
         * @return True when no mappings are active.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Returns diagnostics from the latest synchronization.
         *
         * @return Read-only synchronization result.
         */
        [[nodiscard]] const PickingSyncResult& last_result() const;

    private:
        /**
         * @brief Allocates an unused compact picking identifier.
         *
         * Released identifiers are reused before the sequential ID range grows.
         *
         * @return Valid picking ID, or invalid when the 24-bit range is exhausted.
         */
        [[nodiscard]] graphics::PickingId allocate_picking_id();

        /**
         * @brief Releases an identifier for later reuse.
         *
         * @param pickingId Identifier no longer assigned to a scene node.
         */
        void release_picking_id(
            graphics::PickingId pickingId
        );

    private:
        std::unordered_map<
            SceneNodeId,
            graphics::PickingId
        > nodeToPicking_{};

        std::unordered_map<
            graphics::u32,
            SceneNodeId
        > pickingToNode_{};

        std::vector<graphics::u32> reusableIds_{};

        graphics::u32 nextPickingId_ = 1;

        PickingSyncResult lastResult_{};
    };

} // namespace locus::editor