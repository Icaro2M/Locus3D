/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/sync/ManufacturingSync.h"
#include "editor/sync/PickingSync.h"
#include "editor/sync/RenderSceneSync.h"

namespace locus::editor {

    class Editor;

    /**
     * @brief Options used by the high-level editor synchronization facade.
     */
    struct EditorSyncOptions {
        /**
         * @brief Options forwarded to RenderSceneSync.
         */
        RenderSceneSyncOptions renderSceneOptions{};

        /**
         * @brief True when synchronized dirty flags should be cleared after success.
         */
        bool clearDirtyFlagsAfterSync = true;

        /**
         * @brief Dirty flags cleared after a successful render scene sync.
         */
        EditorDirtyFlags renderCleanFlags =
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking;
    };

    /**
     * @brief Diagnostics produced by EditorSync.
     */
    struct EditorSyncResult {
        /**
         * @brief True when render scene synchronization ran.
         */
        bool renderSceneSynced = false;

        /**
         * @brief True when editor dirty flags were cleared.
         */
        bool dirtyFlagsCleared = false;

        /**
         * @brief Render scene sync diagnostics.
         */
        RenderSceneSyncResult renderSceneResult{};

        /**
         * @brief Human-readable diagnostic message.
         */
        std::string message;
    };

    /**
     * @brief High-level synchronization facade for editor-side derived data.
     *
     * EditorSync coordinates render-scene and picking synchronization.
     */
    class EditorSync {
    public:
        /**
         * @brief Creates an editor sync facade.
         */
        EditorSync() = default;

        /**
         * @brief Destroys the editor sync facade.
         */
        ~EditorSync() = default;

        EditorSync(const EditorSync&) = default;
        EditorSync& operator=(const EditorSync&) = default;
        EditorSync(EditorSync&&) noexcept = default;
        EditorSync& operator=(EditorSync&&) noexcept = default;

        /**
         * @brief Returns render scene synchronization state.
         *
         * @return Mutable RenderSceneSync reference.
         */
        [[nodiscard]] RenderSceneSync& render_scene_sync();

        /**
         * @brief Returns render scene synchronization state.
         *
         * @return Read-only RenderSceneSync reference.
         */
        [[nodiscard]] const RenderSceneSync& render_scene_sync() const;

        /**
         * @brief Returns the latest synchronized graphics render scene.
         *
         * @return Read-only render scene reference.
         */
        [[nodiscard]] const graphics::RenderScene& render_scene() const;

        /**
         * @brief Returns manufacturing analysis synchronization state.
         *
         * @return Mutable manufacturing sync reference.
         */
        [[nodiscard]] ManufacturingSync& manufacturing_sync();

        /**
         * @brief Returns manufacturing analysis synchronization state.
         *
         * @return Read-only manufacturing sync reference.
         */
        [[nodiscard]] const ManufacturingSync& manufacturing_sync() const;

        /**
         * @brief Returns editor-to-graphics picking identifier mappings.
         *
         * @return Mutable picking synchronization state.
         */
        [[nodiscard]] PickingSync& picking_sync();

        /**
         * @brief Returns editor-to-graphics picking identifier mappings.
         *
         * @return Read-only picking synchronization state.
         */
        [[nodiscard]] const PickingSync& picking_sync() const;

        /**
         * @brief Returns diagnostics from the latest editor sync pass.
         *
         * @return Read-only result reference.
         */
        [[nodiscard]] const EditorSyncResult& last_result() const;

        /**
         * @brief Clears all synchronized derived data.
         */
        void clear();

        /**
         * @brief Synchronizes editor state using CPU-only render scene adapters.
         *
         * @param editor Editor facade to synchronize.
         * @param options Sync options.
         * @return True when render scene sync rebuilt data.
         */
        bool sync_cpu_if_needed(
            Editor& editor,
            const EditorSyncOptions& options = {}
        );

        /**
         * @brief Synchronizes editor state using GPU mesh cache and uploader.
         *
         * @param editor Editor facade to synchronize.
         * @param cache Mesh render cache used to own/reuse GPU meshes.
         * @param uploader Mesh uploader used on cache misses.
         * @param options Sync options.
         * @return Empty success result, or graphics error on failure.
         */
        [[nodiscard]] graphics::GraphicsResult<void> sync_cached_if_needed(
            Editor& editor,
            graphics::MeshRenderCache& cache,
            const graphics::MeshUploader& uploader,
            const EditorSyncOptions& options = {}
        );

    private:
        RenderSceneSync renderSceneSync_{};
        ManufacturingSync manufacturingSync_{};
        PickingSync pickingSync_{};
        EditorSyncResult lastResult_{};
    };

} // namespace locus::editor
