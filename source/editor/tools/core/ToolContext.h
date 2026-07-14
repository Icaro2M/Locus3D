/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/Editor.h"

namespace locus::editor {

    /**
     * @brief Editor access boundary provided to active tools.
     *
     * Tools should use this context instead of storing permanent references to
     * editor subsystems. External rendering, picking, platform input, and other
     * application services remain outside this initial context until concrete
     * integration requirements are introduced.
     */
    class ToolContext {
    public:
        /**
         * @brief Creates a tool context.
         *
         * @param editor Editor facade used by tools.
         */
        explicit ToolContext(Editor& editor)
            : editor_(&editor) {
        }

        /**
         * @brief Returns the editor facade.
         *
         * @return Mutable editor reference.
         */
        [[nodiscard]] Editor& editor() {
            return *editor_;
        }

        /**
         * @brief Returns the editor facade.
         *
         * @return Read-only editor reference.
         */
        [[nodiscard]] const Editor& editor() const {
            return *editor_;
        }

        /**
         * @brief Returns mutable editor state.
         *
         * @return Mutable state reference.
         */
        [[nodiscard]] EditorState& state() {
            return editor_->state();
        }

        /**
         * @brief Returns read-only editor state.
         *
         * @return Read-only state reference.
         */
        [[nodiscard]] const EditorState& state() const {
            return editor_->state();
        }

        /**
         * @brief Returns mutable editor scene.
         *
         * @return Mutable scene reference.
         */
        [[nodiscard]] EditorScene& scene() {
            return editor_->scene();
        }

        /**
         * @brief Returns read-only editor scene.
         *
         * @return Read-only scene reference.
         */
        [[nodiscard]] const EditorScene& scene() const {
            return editor_->scene();
        }

        /**
         * @brief Returns mutable selection state.
         *
         * @return Mutable selection state reference.
         */
        [[nodiscard]] SelectionState& selection() {
            return editor_->selection();
        }

        /**
         * @brief Returns read-only selection state.
         *
         * @return Read-only selection state reference.
         */
        [[nodiscard]] const SelectionState& selection() const {
            return editor_->selection();
        }

        /**
         * @brief Returns the high-level selection controller.
         *
         * @return Mutable selection controller reference.
         */
        [[nodiscard]] SelectionController& selection_controller() {
            return editor_->selection_controller();
        }

        /**
         * @brief Returns the high-level selection controller.
         *
         * @return Read-only selection controller reference.
         */
        [[nodiscard]] const SelectionController&
            selection_controller() const {

            return editor_->selection_controller();
        }

        /**
         * @brief Returns mutable snapping settings.
         *
         * @return Mutable snapping settings reference.
         */
        [[nodiscard]] SnapSettings& snap_settings() {
            return editor_->snap_settings();
        }

        /**
         * @brief Returns read-only snapping settings.
         *
         * @return Read-only snapping settings reference.
         */
        [[nodiscard]] const SnapSettings& snap_settings() const {
            return editor_->snap_settings();
        }

        /**
         * @brief Returns the current high-level editor mode.
         *
         * @return Current editor mode.
         */
        [[nodiscard]] EditorMode mode() const {
            return editor_->mode();
        }

        /**
         * @brief Marks editor subsystems as dirty.
         *
         * @param flags Dirty flags to add.
         */
        void mark_dirty(EditorDirtyFlags flags) {
            editor_->mark_dirty(flags);
        }

    private:
        Editor* editor_ = nullptr;
    };

} // namespace locus::editor