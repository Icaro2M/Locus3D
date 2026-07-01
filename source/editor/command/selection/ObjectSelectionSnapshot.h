/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/SceneNodeId.h"
#include "editor/selection/SelectionState.h"

#include <vector>

namespace locus::editor {

	/**
	 * @brief Lightweight snapshot of object-level selection state.
	 */
	class ObjectSelectionSnapshot {
	public:
		/**
		 * @brief Captures object selection state.
		 *
		 * @param selection Selection state to capture.
		 */
		void capture(const SelectionState& selection)
		{
			selected_ = selection.objects().selected();
			active_ = selection.objects().active();
			hovered_ = selection.objects().hovered();
			hasSnapshot_ = true;
		}

		/**
		 * @brief Restores object selection state.
		 *
		 * @param selection Selection state to restore.
		 */
		void restore(SelectionState& selection) const
		{
			if (!hasSnapshot_) {
				return;
			}

			ObjectSelection& objects = selection.objects();
			objects.set(selected_, active_);
			objects.set_hovered(hovered_);
			selection.mark_dirty();
		}

		/**
		 * @brief Checks whether this snapshot contains captured data.
		 *
		 * @return True when captured.
		 */
		[[nodiscard]] bool is_valid() const
		{
			return hasSnapshot_;
		}

	private:
		std::vector<SceneNodeId> selected_{};
		SceneNodeId active_{};
		SceneNodeId hovered_{};
		bool hasSnapshot_ = false;
	};

}