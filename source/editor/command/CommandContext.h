/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/Editor.h"

namespace locus::editor {

	/**
	 * @brief Execution context provided to editor commands.
	 *
	 * Commands should use this context instead of storing direct references to
	 * editor subsystems. This keeps command execution explicit and makes future
	 * history, tools, and scripting integration easier to route through the same
	 * boundary.
	 */
	class CommandContext {
	public:
		/**
		 * @brief Creates a command context.
		 *
		 * @param editor Editor facade used by commands.
		 */
		explicit CommandContext(Editor& editor)
			: editor_(&editor)
		{
		}

		/**
		 * @brief Returns the editor facade.
		 *
		 * @return Mutable editor reference.
		 */
		[[nodiscard]] Editor& editor()
		{
			return *editor_;
		}

		/**
		 * @brief Returns the editor facade.
		 *
		 * @return Read-only editor reference.
		 */
		[[nodiscard]] const Editor& editor() const
		{
			return *editor_;
		}

		/**
		 * @brief Returns mutable editor state.
		 *
		 * @return Mutable state reference.
		 */
		[[nodiscard]] EditorState& state()
		{
			return editor_->state();
		}

		/**
		 * @brief Returns read-only editor state.
		 *
		 * @return Read-only state reference.
		 */
		[[nodiscard]] const EditorState& state() const
		{
			return editor_->state();
		}

		/**
		 * @brief Returns mutable editor scene.
		 *
		 * @return Mutable scene reference.
		 */
		[[nodiscard]] EditorScene& scene()
		{
			return editor_->scene();
		}

		/**
		 * @brief Returns read-only editor scene.
		 *
		 * @return Read-only scene reference.
		 */
		[[nodiscard]] const EditorScene& scene() const
		{
			return editor_->scene();
		}

		/**
		 * @brief Returns mutable selection state.
		 *
		 * @return Mutable selection state reference.
		 */
		[[nodiscard]] SelectionState& selection()
		{
			return editor_->selection();
		}

		/**
		 * @brief Returns read-only selection state.
		 *
		 * @return Read-only selection state reference.
		 */
		[[nodiscard]] const SelectionState& selection() const
		{
			return editor_->selection();
		}

		/**
		 * @brief Returns the high-level selection controller.
		 *
		 * @return Selection controller reference.
		 */
		[[nodiscard]] SelectionController& selection_controller()
		{
			return editor_->selection_controller();
		}

		/**
		 * @brief Marks editor subsystems as dirty.
		 *
		 * @param flags Dirty flags to add.
		 */
		void mark_dirty(EditorDirtyFlags flags)
		{
			editor_->mark_dirty(flags);
		}

	private:
		Editor* editor_ = nullptr;
	};

}