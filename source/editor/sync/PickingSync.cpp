/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/sync/PickingSync.h"

#include "editor/scene/EditorScene.h"

#include <unordered_set>
#include <utility>

namespace locus::editor {

    void PickingSync::clear() {
        nodeToPicking_.clear();
        pickingToNode_.clear();
        reusableIds_.clear();

        nextPickingId_ = 1;
        lastResult_ = {};
    }

    bool PickingSync::sync(const EditorScene& scene) {
        lastResult_ = {};

        const std::vector<SceneNodeId> sceneNodeIds =
            scene.tree().node_ids();

        lastResult_.sceneNodeCount = sceneNodeIds.size();

        std::unordered_set<SceneNodeId> activeNodeIds;
        activeNodeIds.reserve(sceneNodeIds.size());

        for (const SceneNodeId nodeId : sceneNodeIds) {
            if (nodeId.is_valid()) {
                activeNodeIds.insert(nodeId);
            }
        }

        for (auto iterator = nodeToPicking_.begin();
            iterator != nodeToPicking_.end();) {
            const SceneNodeId nodeId = iterator->first;

            if (activeNodeIds.find(nodeId)
                != activeNodeIds.end()) {
                ++lastResult_.preservedMappingCount;
                ++iterator;
                continue;
            }

            const graphics::PickingId pickingId =
                iterator->second;

            pickingToNode_.erase(pickingId.value);
            release_picking_id(pickingId);

            iterator = nodeToPicking_.erase(iterator);

            ++lastResult_.removedMappingCount;
        }

        bool synchronized = true;

        for (const SceneNodeId nodeId : sceneNodeIds) {
            if (!nodeId.is_valid()) {
                continue;
            }

            if (nodeToPicking_.find(nodeId)
                != nodeToPicking_.end()) {
                continue;
            }

            const graphics::PickingId pickingId =
                allocate_picking_id();

            if (!pickingId.is_valid()) {
                synchronized = false;
                lastResult_.exhausted = true;
                continue;
            }

            nodeToPicking_.emplace(nodeId, pickingId);
            pickingToNode_.emplace(pickingId.value, nodeId);

            ++lastResult_.createdMappingCount;
        }

        lastResult_.mappingCount = nodeToPicking_.size();
        lastResult_.synchronized = synchronized;

        if (lastResult_.exhausted) {
            lastResult_.message =
                "Picking synchronization could not assign an ID "
                "to every scene node because the 24-bit ID range "
                "was exhausted.";
        }
        else if (sceneNodeIds.empty()) {
            lastResult_.message =
                "Picking synchronization completed with an empty scene.";
        }
        else {
            lastResult_.message =
                "Picking identifiers synchronized successfully.";
        }

        return synchronized;
    }

    bool PickingSync::contains(
        const SceneNodeId nodeId
    ) const {
        if (!nodeId.is_valid()) {
            return false;
        }

        return nodeToPicking_.find(nodeId)
            != nodeToPicking_.end();
    }

    bool PickingSync::contains(
        const graphics::PickingId pickingId
    ) const {
        if (!pickingId.is_valid()) {
            return false;
        }

        return pickingToNode_.find(pickingId.value)
            != pickingToNode_.end();
    }

    graphics::PickingId PickingSync::picking_id(
        const SceneNodeId nodeId
    ) const {
        if (!nodeId.is_valid()) {
            return graphics::PickingId::invalid();
        }

        const auto iterator = nodeToPicking_.find(nodeId);

        if (iterator == nodeToPicking_.end()) {
            return graphics::PickingId::invalid();
        }

        return iterator->second;
    }

    SceneNodeId PickingSync::scene_node_id(
        const graphics::PickingId pickingId
    ) const {
        if (!pickingId.is_valid()) {
            return SceneNodeId{};
        }

        const auto iterator =
            pickingToNode_.find(pickingId.value);

        if (iterator == pickingToNode_.end()) {
            return SceneNodeId{};
        }

        return iterator->second;
    }

    std::size_t PickingSync::size() const {
        return nodeToPicking_.size();
    }

    bool PickingSync::empty() const {
        return nodeToPicking_.empty();
    }

    const PickingSyncResult& PickingSync::last_result() const {
        return lastResult_;
    }

    graphics::PickingId PickingSync::allocate_picking_id() {
        if (!reusableIds_.empty()) {
            const graphics::u32 value =
                reusableIds_.back();

            reusableIds_.pop_back();

            return graphics::PickingId::from_u32(value);
        }

        if (nextPickingId_ == 0
            || nextPickingId_ > MaxPickingId) {
            return graphics::PickingId::invalid();
        }

        const graphics::PickingId result =
            graphics::PickingId::from_u32(
                nextPickingId_
            );

        ++nextPickingId_;

        return result;
    }

    void PickingSync::release_picking_id(
        const graphics::PickingId pickingId
    ) {
        if (!pickingId.is_valid()
            || pickingId.value > MaxPickingId) {
            return;
        }

        reusableIds_.push_back(pickingId.value);
    }

} // namespace locus::editor