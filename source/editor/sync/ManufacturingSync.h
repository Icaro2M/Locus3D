/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/manufacturing/ManufacturingDisplaySettings.h"
#include "editor/scene/SceneNodeId.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/manufacturing/core/AnalysisReport.h"
#include "kernel/manufacturing/pipeline/AnalysisOptions.h"
#include "kernel/manufacturing/profiles/PrintProfile.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/mat4x4.hpp>

namespace locus::editor {

    class Editor;
    class EditorScene;
    class MeshNode;

    /**
     * @brief Configuration that affects manufacturing analysis results.
     */
    struct ManufacturingAnalysisSettings {
        kernel::manufacturing::PrintProfile profile;
        kernel::manufacturing::AnalysisOptions options{};
        std::uint64_t revision = 1;

        /**
         * @brief Creates default FDM settings for manual viewport diagnostics.
         */
        ManufacturingAnalysisSettings();
    };

    /**
     * @brief Cached manufacturing report for one mesh scene node.
     */
    struct ManufacturingNodeResult {
        SceneNodeId nodeId{};
        std::uint64_t meshRevision = 0;
        std::uint64_t analysisRevision = 0;
        glm::mat4 worldMatrix{ 1.0f };
        kernel::manufacturing::AnalysisReport report{};
        bool valid = false;
    };

    /**
     * @brief Diagnostics from one manufacturing synchronization pass.
     */
    struct ManufacturingSyncResult {
        bool enabled = false;
        bool rebuiltAny = false;
        std::size_t visitedNodeCount = 0;
        std::size_t analyzedNodeCount = 0;
        std::size_t reusedNodeCount = 0;
        std::size_t removedNodeCount = 0;
        std::size_t issueCount = 0;
        std::string message;
    };

    /**
     * @brief Synchronizes editor mesh nodes into manufacturing analysis reports.
     */
    class ManufacturingSync {
    public:
        ManufacturingSync() = default;
        ~ManufacturingSync() = default;

        ManufacturingSync(const ManufacturingSync&) = default;
        ManufacturingSync& operator=(const ManufacturingSync&) = default;
        ManufacturingSync(ManufacturingSync&&) noexcept = default;
        ManufacturingSync& operator=(ManufacturingSync&&) noexcept = default;

        /**
         * @brief Returns analysis settings used for future sync passes.
         */
        [[nodiscard]] ManufacturingAnalysisSettings& analysis_settings() noexcept;

        /**
         * @brief Returns analysis settings used for future sync passes.
         */
        [[nodiscard]] const ManufacturingAnalysisSettings& analysis_settings() const noexcept;

        /**
         * @brief Returns display settings used by viewport adapters.
         */
        [[nodiscard]] ManufacturingDisplaySettings& display_settings() noexcept;

        /**
         * @brief Returns display settings used by viewport adapters.
         */
        [[nodiscard]] const ManufacturingDisplaySettings& display_settings() const noexcept;

        /**
         * @brief Toggles manufacturing diagnostics visibility.
         */
        void set_enabled(bool enabled) noexcept;

        /**
         * @brief Checks whether manufacturing diagnostics are enabled.
         */
        [[nodiscard]] bool enabled() const noexcept;

        /**
         * @brief Marks analysis configuration as changed.
         */
        void bump_analysis_revision() noexcept;

        /**
         * @brief Clears cached reports and diagnostics.
         */
        void clear();

        /**
         * @brief Synchronizes analysis reports for visible mesh nodes.
         */
        const ManufacturingSyncResult& sync_if_needed(const Editor& editor);

        /**
         * @brief Returns cached results in deterministic node order.
         */
        [[nodiscard]] std::vector<const ManufacturingNodeResult*> results() const;

        /**
         * @brief Returns diagnostics from the latest sync pass.
         */
        [[nodiscard]] const ManufacturingSyncResult& last_result() const noexcept;

    private:
        [[nodiscard]] bool needs_rebuild(
            const ManufacturingNodeResult& cached,
            const MeshNode& node,
            const glm::mat4& worldMatrix) const noexcept;

        [[nodiscard]] static kernel::geometry::LEM build_world_mesh(
            const MeshNode& node,
            const glm::mat4& worldMatrix);

        void remove_stale_results(
            const EditorScene& scene,
            ManufacturingSyncResult& result);

        ManufacturingAnalysisSettings analysisSettings_{};
        ManufacturingDisplaySettings displaySettings_{};
        std::unordered_map<SceneNodeId, ManufacturingNodeResult> nodeResults_{};
        ManufacturingSyncResult lastResult_{};
    };

} // namespace locus::editor
