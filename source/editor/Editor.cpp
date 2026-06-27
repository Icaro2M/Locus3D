/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"

namespace locus::editor {

    Editor::Editor() = default;

    Editor::Editor(EditorContext context)
        : context_(context)
    {
    }

    EditorState& Editor::state()
    {
        return state_;
    }

    const EditorState& Editor::state() const
    {
        return state_;
    }

    EditorContext& Editor::context()
    {
        return context_;
    }

    const EditorContext& Editor::context() const
    {
        return context_;
    }

    EditorScene& Editor::scene()
    {
        mark_dirty(EditorDirtyFlags::Scene);
        return state_.scene;
    }

    const EditorScene& Editor::scene() const
    {
        return state_.scene;
    }

    void Editor::set_mode(EditorMode mode)
    {
        if (state_.mode == mode) {
            return;
        }

        state_.mode = mode;
        mark_dirty(EditorDirtyFlags::Selection);
    }

    EditorMode Editor::mode() const
    {
        return state_.mode;
    }

    void Editor::mark_dirty(EditorDirtyFlags flags)
    {
        state_.dirtyFlags |= flags;
    }

    void Editor::clear_dirty(EditorDirtyFlags flags)
    {
        state_.dirtyFlags = static_cast<EditorDirtyFlags>(
            static_cast<std::uint32_t>(state_.dirtyFlags) &
            ~static_cast<std::uint32_t>(flags));
    }

    EditorDirtyFlags Editor::dirty_flags() const
    {
        return state_.dirtyFlags;
    }

}