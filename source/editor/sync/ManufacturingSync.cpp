/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/sync/ManufacturingSync.h"

#include "editor/Editor.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneTransforms.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/pipeline/AnalysisPipeline.h"
#include "kernel/manufacturing/profiles/FDMProfile.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include <glm/vec4.hpp>

namespace locus::editor {

    namespace {

        [[nodiscard]] kernel::manufacturing::PrintProfile make_default_profile()
        {
            kernel::manufacturing::FDMProfile profile{};
            profile.name = "Viewport FDM diagnostic profile";
            profile.limits.minimumWallThickness = 0.8;
            profile.limits.minimumFeatureSize = 0.4;
            profile.limits.maximumUnsupportedOverhangAngleDegrees = 45.0;
            profile.nozzleDiameter = 0.4;
            profile.extrusionWidth = 0.45;
            profile.layerHeight = 0.2;
            return kernel::manufacturing::PrintProfile{ std::move(profile) };
        }

        [[nodiscard]] bool matrices_nearly_equal(
            const glm::mat4& lhs,
            const glm::mat4& rhs) noexcept
        {
            constexpr float Epsilon = 0.00001f;

            for (int column = 0; column < 4; ++column) {
                for (int row = 0; row < 4; ++row) {
                    if (std::fabs(lhs[column][row] - rhs[column][row]) > Epsilon) {
                        return false;
                    }
                }
            }

            return true;
        }

        [[nodiscard]] glm::vec3 transform_point(
            const glm::mat4& matrix,
            const glm::vec3& point)
        {
            return glm::vec3{ matrix * glm::vec4{ point, 1.0f } };
        }

    } // namespace

    ManufacturingAnalysisSettings::ManufacturingAnalysisSettings()
        : profile(make_default_profile())
    {
        options.thinWallQuality =
            kernel::manufacturing::ThinWallQuality::Balanced;
    }

    ManufacturingAnalysisSettings& ManufacturingSync::analysis_settings() noexcept
    {
        return analysisSettings_;
    }

    const ManufacturingAnalysisSettings& ManufacturingSync::analysis_settings() const noexcept
    {
        return analysisSettings_;
    }

    ManufacturingDisplaySettings& ManufacturingSync::display_settings() noexcept
    {
        return displaySettings_;
    }

    const ManufacturingDisplaySettings& ManufacturingSync::display_settings() const noexcept
    {
        return displaySettings_;
    }

    void ManufacturingSync::set_enabled(bool enabled) noexcept
    {
        displaySettings_.enabled = enabled;
    }

    bool ManufacturingSync::enabled() const noexcept
    {
        return displaySettings_.enabled;
    }

    void ManufacturingSync::bump_analysis_revision() noexcept
    {
        ++analysisSettings_.revision;
    }

    void ManufacturingSync::clear()
    {
        nodeResults_.clear();
        lastResult_ = {};
        lastResult_.message = "Manufacturing sync state cleared.";
    }

    const ManufacturingSyncResult& ManufacturingSync::sync_if_needed(
        const Editor& editor)
    {
        lastResult_ = {};
        lastResult_.enabled = enabled();

        if (!enabled()) {
            lastResult_.message = "Manufacturing sync skipped because display is disabled.";
            return lastResult_;
        }

        const EditorScene& scene = editor.scene();
        const std::vector<SceneNodeId> nodeIds = scene.tree().node_ids();

        for (SceneNodeId nodeId : nodeIds) {
            const MeshNode* meshNode = scene.find_mesh(nodeId);
            if (meshNode == nullptr || !meshNode->is_visible() || meshNode->mesh().empty()) {
                continue;
            }

            ++lastResult_.visitedNodeCount;

            const glm::mat4 worldMatrix =
                SceneTransforms::world_matrix(scene, nodeId);

            ManufacturingNodeResult& cached = nodeResults_[nodeId];

            if (!needs_rebuild(cached, *meshNode, worldMatrix)) {
                ++lastResult_.reusedNodeCount;
                lastResult_.issueCount += cached.report.issue_count();
                continue;
            }

            kernel::geometry::LEM worldMesh =
                build_world_mesh(*meshNode, worldMatrix);

            cached = {};
            cached.nodeId = nodeId;
            cached.meshRevision = meshNode->mesh_revision();
            cached.analysisRevision = analysisSettings_.revision;
            cached.worldMatrix = worldMatrix;
            cached.report = kernel::manufacturing::AnalysisPipeline::analyze(
                worldMesh,
                analysisSettings_.profile,
                analysisSettings_.options);
            cached.valid = true;

            ++lastResult_.analyzedNodeCount;
            lastResult_.rebuiltAny = true;
            lastResult_.issueCount += cached.report.issue_count();
        }

        remove_stale_results(scene, lastResult_);

        lastResult_.message = lastResult_.rebuiltAny
            ? "Manufacturing reports synchronized."
            : "Manufacturing reports reused from cache.";

        return lastResult_;
    }

    std::vector<const ManufacturingNodeResult*> ManufacturingSync::results() const
    {
        std::vector<const ManufacturingNodeResult*> ordered;
        ordered.reserve(nodeResults_.size());

        for (const auto& entry : nodeResults_) {
            if (entry.second.valid) {
                ordered.push_back(&entry.second);
            }
        }

        std::sort(
            ordered.begin(),
            ordered.end(),
            [](const ManufacturingNodeResult* lhs, const ManufacturingNodeResult* rhs) {
                return lhs->nodeId.value < rhs->nodeId.value;
            });

        return ordered;
    }

    const ManufacturingSyncResult& ManufacturingSync::last_result() const noexcept
    {
        return lastResult_;
    }

    bool ManufacturingSync::needs_rebuild(
        const ManufacturingNodeResult& cached,
        const MeshNode& node,
        const glm::mat4& worldMatrix) const noexcept
    {
        return !cached.valid ||
            cached.meshRevision != node.mesh_revision() ||
            cached.analysisRevision != analysisSettings_.revision ||
            !matrices_nearly_equal(cached.worldMatrix, worldMatrix);
    }

    kernel::geometry::LEM ManufacturingSync::build_world_mesh(
        const MeshNode& node,
        const glm::mat4& worldMatrix)
    {
        kernel::geometry::LEM mesh = node.mesh();

        for (const kernel::geometry::VertexHandle vertex :
            kernel::geometry::TopologyTraversal::vertices(mesh)) {

            mesh.vertex(vertex).position =
                transform_point(worldMatrix, mesh.vertex(vertex).position);
        }

        return mesh;
    }

    void ManufacturingSync::remove_stale_results(
        const EditorScene& scene,
        ManufacturingSyncResult& result)
    {
        for (auto it = nodeResults_.begin(); it != nodeResults_.end();) {
            const MeshNode* meshNode = scene.find_mesh(it->first);
            if (meshNode != nullptr && meshNode->is_visible() && !meshNode->mesh().empty()) {
                ++it;
                continue;
            }

            it = nodeResults_.erase(it);
            ++result.removedNodeCount;
        }
    }

} // namespace locus::editor
