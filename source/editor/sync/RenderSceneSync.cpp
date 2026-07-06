/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/sync/RenderSceneSync.h"

#include "editor/Editor.h"
#include "editor/scene/EditorScene.h"
#include "editor/selection/SelectionState.h"
#include "graphics/common/GraphicsError.h"

namespace locus::editor {

    const graphics::RenderScene& RenderSceneSync::render_scene() const
    {
        return renderScene_;
    }

    const RenderSceneSyncResult& RenderSceneSync::last_result() const
    {
        return lastResult_;
    }

    void RenderSceneSync::clear()
    {
        renderScene_.clear();
        lastResult_ = {};
        lastResult_.message = "Render scene sync state cleared.";
    }

    bool RenderSceneSync::should_rebuild(
        EditorDirtyFlags dirtyFlags,
        const RenderSceneSyncOptions& options
    ) {
        return has_flag(dirtyFlags, options.rebuildFlags);
    }

    const graphics::RenderScene& RenderSceneSync::rebuild_cpu(
        const EditorScene& scene,
        const SelectionState& selection,
        const RenderSceneSyncOptions& options
    ) {
        lastResult_ = {};
        lastResult_.rebuilt = true;
        lastResult_.usedGpuCache = false;

        SceneRenderOptions sceneOptions = options.sceneOptions;
        sceneOptions.allowNullGpuMeshes = true;

        graphics::RenderScene baseScene =
            SceneRenderAdapter::build_render_scene(
                scene,
                {},
                sceneOptions,
                &lastResult_.sceneResult
            );

        if (options.applySelection) {
            renderScene_ =
                SelectionRenderAdapter::apply_selection(
                    baseScene,
                    selection,
                    options.selectionOptions,
                    &lastResult_.selectionResult
                );

            lastResult_.selectionApplied = true;
        }
        else {
            renderScene_ = std::move(baseScene);
        }

        lastResult_.objectCount = renderScene_.object_count();
        lastResult_.message = "Render scene rebuilt using CPU-only sync path.";

        return renderScene_;
    }

    const graphics::RenderScene& RenderSceneSync::rebuild_cpu(
        const Editor& editor,
        const RenderSceneSyncOptions& options
    ) {
        lastResult_.inputDirtyFlags = editor.dirty_flags();

        const graphics::RenderScene& scene =
            rebuild_cpu(editor.scene(), editor.selection(), options);

        lastResult_.inputDirtyFlags = editor.dirty_flags();
        return scene;
    }

    graphics::GraphicsResult<void> RenderSceneSync::rebuild_cached(
        const EditorScene& scene,
        const SelectionState& selection,
        graphics::MeshRenderCache& cache,
        const graphics::MeshUploader& uploader,
        const RenderSceneSyncOptions& options
    ) {
        lastResult_ = {};
        lastResult_.rebuilt = true;
        lastResult_.usedGpuCache = true;

        SceneRenderOptions sceneOptions = options.sceneOptions;
        sceneOptions.allowNullGpuMeshes = false;

        auto sceneResult =
            SceneRenderAdapter::build_cached_render_scene(
                scene,
                cache,
                uploader,
                sceneOptions,
                &lastResult_.sceneResult
            );

        if (!sceneResult) {
            renderScene_.clear();
            lastResult_.objectCount = 0;
            lastResult_.message = sceneResult.error().message;
            return sceneResult.error();
        }

        graphics::RenderScene baseScene = sceneResult.move_value();

        if (options.applySelection) {
            renderScene_ =
                SelectionRenderAdapter::apply_selection(
                    baseScene,
                    selection,
                    options.selectionOptions,
                    &lastResult_.selectionResult
                );

            lastResult_.selectionApplied = true;
        }
        else {
            renderScene_ = std::move(baseScene);
        }

        lastResult_.objectCount = renderScene_.object_count();
        lastResult_.message = "Render scene rebuilt using GPU cache sync path.";

        return {};
    }

    graphics::GraphicsResult<void> RenderSceneSync::rebuild_cached(
        const Editor& editor,
        graphics::MeshRenderCache& cache,
        const graphics::MeshUploader& uploader,
        const RenderSceneSyncOptions& options
    ) {
        lastResult_.inputDirtyFlags = editor.dirty_flags();

        auto result =
            rebuild_cached(editor.scene(), editor.selection(), cache, uploader, options);

        lastResult_.inputDirtyFlags = editor.dirty_flags();
        return result;
    }

    bool RenderSceneSync::sync_cpu_if_needed(
        const Editor& editor,
        const RenderSceneSyncOptions& options
    ) {
        const EditorDirtyFlags dirtyFlags = editor.dirty_flags();

        if (!should_rebuild(dirtyFlags, options)) {
            lastResult_ = {};
            lastResult_.rebuilt = false;
            lastResult_.inputDirtyFlags = dirtyFlags;
            lastResult_.objectCount = renderScene_.object_count();
            lastResult_.message = "Render scene sync skipped because no render-relevant dirty flags were set.";
            return false;
        }

        rebuild_cpu(editor, options);
        lastResult_.inputDirtyFlags = dirtyFlags;
        return true;
    }

    graphics::GraphicsResult<void> RenderSceneSync::sync_cached_if_needed(
        const Editor& editor,
        graphics::MeshRenderCache& cache,
        const graphics::MeshUploader& uploader,
        const RenderSceneSyncOptions& options
    ) {
        const EditorDirtyFlags dirtyFlags = editor.dirty_flags();

        if (!should_rebuild(dirtyFlags, options)) {
            lastResult_ = {};
            lastResult_.rebuilt = false;
            lastResult_.inputDirtyFlags = dirtyFlags;
            lastResult_.objectCount = renderScene_.object_count();
            lastResult_.message = "Render scene sync skipped because no render-relevant dirty flags were set.";
            return {};
        }

        auto result = rebuild_cached(editor, cache, uploader, options);
        lastResult_.inputDirtyFlags = dirtyFlags;
        return result;
    }

} // namespace locus::editor