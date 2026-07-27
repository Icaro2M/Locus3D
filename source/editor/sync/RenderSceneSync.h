/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/EditorTypes.h"
#include "editor/render/SceneRenderAdapter.h"
#include "editor/render/SelectionRenderAdapter.h"
#include "graphics/graphics.h"

#include <string>

namespace locus::editor {

    class Editor;
    class EditorScene;
    class SelectionState;

    /**
     * @brief Options used when synchronizing editor state to a graphics render scene.
     */
    struct RenderSceneSyncOptions {
        /**
         * @brief Options used when converting the editor scene to a render scene.
         */
        SceneRenderOptions sceneOptions{};

        /**
         * @brief Options used when applying editor selection to render objects.
         */
        SelectionRenderOptions selectionOptions{};

        /**
         * @brief True when selection flags should be applied after scene conversion.
         */
        bool applySelection = true;

        /**
         * @brief Dirty flags that require a full render scene rebuild.
         */
        EditorDirtyFlags rebuildFlags =
            EditorDirtyFlags::Scene |
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking;
    };

    /**
     * @brief Diagnostics produced by RenderSceneSync.
     */
    struct RenderSceneSyncResult {
        /**
         * @brief True when the render scene was rebuilt in this sync pass.
         */
        bool rebuilt = false;

        /**
         * @brief True when selection flags were applied in this sync pass.
         */
        bool selectionApplied = false;

        /**
         * @brief True when a cached GPU-backed scene path was used.
         */
        bool usedGpuCache = false;

        /**
         * @brief Dirty flags observed before synchronization.
         */
        EditorDirtyFlags inputDirtyFlags = EditorDirtyFlags::None;

        /**
         * @brief Number of objects in the final synchronized render scene.
         */
        std::size_t objectCount = 0;

        /**
         * @brief Diagnostics from SceneRenderAdapter.
         */
        SceneRenderResult sceneResult{};

        /**
         * @brief Diagnostics from SelectionRenderAdapter.
         */
        SelectionRenderResult selectionResult{};

        /**
         * @brief Human-readable diagnostic message.
         */
        std::string message;
    };

    /**
     * @brief Synchronizes editor scene and selection state into a graphics render scene.
     *
     * RenderSceneSync decides when and how to rebuild the render-side scene. The
     * actual semantic conversions remain delegated to editor/render adapters.
     */
    class RenderSceneSync {
    public:
        /**
         * @brief Creates an empty render scene synchronizer.
         */
        RenderSceneSync() = default;

        /**
         * @brief Destroys the synchronizer.
         */
        ~RenderSceneSync() = default;

        RenderSceneSync(const RenderSceneSync&) = default;
        RenderSceneSync& operator=(const RenderSceneSync&) = default;
        RenderSceneSync(RenderSceneSync&&) noexcept = default;
        RenderSceneSync& operator=(RenderSceneSync&&) noexcept = default;

        /**
         * @brief Returns the latest synchronized render scene.
         *
         * @return Read-only render scene reference.
         */
        [[nodiscard]] const graphics::RenderScene& render_scene() const;

        /**
         * @brief Returns the latest synchronized render scene for derived-data adapters.
         *
         * @return Mutable render scene reference.
         */
        [[nodiscard]] graphics::RenderScene& render_scene();

        /**
         * @brief Returns diagnostics from the latest synchronization.
         *
         * @return Read-only result reference.
         */
        [[nodiscard]] const RenderSceneSyncResult& last_result() const;

        /**
         * @brief Clears the synchronized render scene and diagnostics.
         */
        void clear();

        /**
         * @brief Checks whether the provided dirty flags require a render scene rebuild.
         *
         * @param dirtyFlags Current editor dirty flags.
         * @param options Sync options.
         * @return True when render scene sync should rebuild.
         */
        [[nodiscard]] static bool should_rebuild(
            EditorDirtyFlags dirtyFlags,
            const RenderSceneSyncOptions& options = {}
        );

        /**
         * @brief Rebuilds the render scene using CPU-only adapters.
         *
         * This path does not require an OpenGL context. Render objects may reference
         * null GPU meshes depending on SceneRenderOptions.
         *
         * @param scene Source editor scene.
         * @param selection Source editor selection state.
         * @param options Sync options.
         * @return Read-only reference to the synchronized render scene.
         */
        const graphics::RenderScene& rebuild_cpu(
            const EditorScene& scene,
            const SelectionState& selection,
            const RenderSceneSyncOptions& options = {}
        );

        /**
         * @brief Rebuilds the render scene from an editor facade using CPU-only adapters.
         *
         * @param editor Source editor facade.
         * @param options Sync options.
         * @return Read-only reference to the synchronized render scene.
         */
        const graphics::RenderScene& rebuild_cpu(
            const Editor& editor,
            const RenderSceneSyncOptions& options = {}
        );

        /**
         * @brief Rebuilds the render scene using MeshRenderCache and MeshUploader.
         *
         * This path requires a valid graphics context before MeshUploader uploads
         * GPU resources.
         *
         * @param scene Source editor scene.
         * @param selection Source editor selection state.
         * @param cache Mesh render cache used to own/reuse GPU meshes.
         * @param uploader Mesh uploader used on cache misses.
         * @param options Sync options.
         * @return Empty success result, or graphics error on failure.
         */
        [[nodiscard]] graphics::GraphicsResult<void> rebuild_cached(
            const EditorScene& scene,
            const SelectionState& selection,
            graphics::MeshRenderCache& cache,
            const graphics::MeshUploader& uploader,
            const RenderSceneSyncOptions& options = {}
        );

        /**
         * @brief Rebuilds the render scene from an editor facade using GPU cache.
         *
         * @param editor Source editor facade.
         * @param cache Mesh render cache used to own/reuse GPU meshes.
         * @param uploader Mesh uploader used on cache misses.
         * @param options Sync options.
         * @return Empty success result, or graphics error on failure.
         */
        [[nodiscard]] graphics::GraphicsResult<void> rebuild_cached(
            const Editor& editor,
            graphics::MeshRenderCache& cache,
            const graphics::MeshUploader& uploader,
            const RenderSceneSyncOptions& options = {}
        );

        /**
         * @brief Rebuilds only when dirty flags require render sync.
         *
         * @param editor Source editor facade.
         * @param options Sync options.
         * @return True when a rebuild happened.
         */
        bool sync_cpu_if_needed(
            const Editor& editor,
            const RenderSceneSyncOptions& options = {}
        );

        /**
         * @brief Rebuilds using GPU cache only when dirty flags require render sync.
         *
         * @param editor Source editor facade.
         * @param cache Mesh render cache used to own/reuse GPU meshes.
         * @param uploader Mesh uploader used on cache misses.
         * @param options Sync options.
         * @return Empty success result, or graphics error on failure.
         */
        [[nodiscard]] graphics::GraphicsResult<void> sync_cached_if_needed(
            const Editor& editor,
            graphics::MeshRenderCache& cache,
            const graphics::MeshUploader& uploader,
            const RenderSceneSyncOptions& options = {}
        );

    private:
        graphics::RenderScene renderScene_{};
        RenderSceneSyncResult lastResult_{};
    };

} // namespace locus::editor
