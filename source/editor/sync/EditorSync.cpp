/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/sync/EditorSync.h"

#include "editor/Editor.h"
#include "editor/render/PickingRenderAdapter.h"

namespace locus::editor {

    RenderSceneSync& EditorSync::render_scene_sync()
    {
        return renderSceneSync_;
    }

    const RenderSceneSync& EditorSync::render_scene_sync() const
    {
        return renderSceneSync_;
    }

    const graphics::RenderScene& EditorSync::render_scene() const
    {
        return renderSceneSync_.render_scene();
    }

    PickingSync& EditorSync::picking_sync()
    {
        return pickingSync_;
    }

    const PickingSync& EditorSync::picking_sync() const
    {
        return pickingSync_;
    }

    const EditorSyncResult& EditorSync::last_result() const
    {
        return lastResult_;
    }

    void EditorSync::clear()
    {
        renderSceneSync_.clear();
        pickingSync_.clear();

        lastResult_ = {};
        lastResult_.message = "Editor sync state cleared.";
    }

    bool EditorSync::sync_cpu_if_needed(
        Editor& editor,
        const EditorSyncOptions& options
    ) {
        lastResult_ = {};

        const bool rebuilt =
            renderSceneSync_.sync_cpu_if_needed(editor, options.renderSceneOptions);

        lastResult_.renderSceneSynced = rebuilt;
        lastResult_.renderSceneResult = renderSceneSync_.last_result();

        if (rebuilt) {
            const Editor& readOnlyEditor = editor;
            (void)pickingSync_.sync(readOnlyEditor.scene());
            PickingRenderAdapter::apply_to_scene(
                renderSceneSync_.render_scene(),
                pickingSync_);
        }

        if (rebuilt && options.clearDirtyFlagsAfterSync) {
            editor.clear_dirty(options.renderCleanFlags);
            lastResult_.dirtyFlagsCleared = true;
        }

        lastResult_.message = rebuilt
            ? "Editor CPU sync completed."
            : "Editor CPU sync skipped because no render-relevant dirty flags were set.";

        return rebuilt;
    }

    graphics::GraphicsResult<void> EditorSync::sync_cached_if_needed(
        Editor& editor,
        graphics::MeshRenderCache& cache,
        const graphics::MeshUploader& uploader,
        const EditorSyncOptions& options
    ) {
        lastResult_ = {};

        auto result =
            renderSceneSync_.sync_cached_if_needed(
                editor,
                cache,
                uploader,
                options.renderSceneOptions
            );
        if (!result) {
            lastResult_.renderSceneSynced = false;
            lastResult_.renderSceneResult = renderSceneSync_.last_result();
            lastResult_.message = result.error().message;
            return result;
        }

        const bool rebuilt = renderSceneSync_.last_result().rebuilt;

        lastResult_.renderSceneSynced = rebuilt;
        lastResult_.renderSceneResult = renderSceneSync_.last_result();

        if (rebuilt) {
            const Editor& readOnlyEditor = editor;
            (void)pickingSync_.sync(readOnlyEditor.scene());
            PickingRenderAdapter::apply_to_scene(
                renderSceneSync_.render_scene(),
                pickingSync_);
        }

        if (rebuilt && options.clearDirtyFlagsAfterSync) {
            editor.clear_dirty(options.renderCleanFlags);
            lastResult_.dirtyFlagsCleared = true;
        }

        lastResult_.message = rebuilt
            ? "Editor GPU cached sync completed."
            : "Editor GPU cached sync skipped because no render-relevant dirty flags were set.";

        return {};
    }

} // namespace locus::editor
