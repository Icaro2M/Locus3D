/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"

namespace locus::editor {

    Editor::Editor()
    {
        rebuild_controllers();
    }

    Editor::Editor(EditorContext context)
        : context_(context)
    {
        rebuild_controllers();
    }

    Editor::~Editor() = default;

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

    SelectionState& Editor::selection()
    {
        mark_dirty(EditorDirtyFlags::Selection);
        return state_.selection;
    }

    const SelectionState& Editor::selection() const
    {
        return state_.selection;
    }

    SelectionController& Editor::selection_controller()
    {
        mark_dirty(EditorDirtyFlags::Selection);
        return *selectionController_;
    }

    const SelectionController& Editor::selection_controller() const
    {
        return *selectionController_;
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

    void Editor::rebuild_controllers()
    {
        selectionController_ =
            std::make_unique<SelectionController>(state_.scene, state_.selection);
    }

}