/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/Editor.h"
#include "editor/command/CommandResult.h"

#include <memory>

namespace locus::editor {

    class CommandDispatcher;
    class HistoryStack;
    class ICommand;

    /**
     * @brief Runtime services and editor access provided to immediate actions.
     *
     * ActionContext does not own any referenced service. The application or
     * editor runtime that creates this context must keep the supplied services
     * alive for the entire period in which the context is used.
     *
     * UI ownership, menu layout, shortcut mapping, command palette state, and
     * platform events remain outside this class.
     */
    class ActionContext {
    public:
        /**
         * @brief Creates a complete action runtime context.
         *
         * @param editor Editor facade used by actions.
         * @param dispatcher Dispatcher used for command execution.
         * @param history Undo and redo history used for persistent commands.
         */
        ActionContext(
            Editor& editor,
            CommandDispatcher& dispatcher,
            HistoryStack& history)
            : editor_(&editor),
            dispatcher_(&dispatcher),
            history_(&history) {
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
         * @brief Returns mutable editor selection state.
         *
         * @return Mutable selection state reference.
         */
        [[nodiscard]] SelectionState& selection() {
            return editor_->selection();
        }

        /**
         * @brief Returns read-only editor selection state.
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
         * @brief Returns the current high-level editor mode.
         *
         * @return Current editor mode.
         */
        [[nodiscard]] EditorMode mode() const {
            return editor_->mode();
        }

        /**
         * @brief Returns the command dispatcher.
         *
         * @return Mutable dispatcher reference.
         */
        [[nodiscard]] CommandDispatcher& dispatcher() {
            return *dispatcher_;
        }

        /**
         * @brief Returns the command dispatcher.
         *
         * @return Read-only dispatcher reference.
         */
        [[nodiscard]] const CommandDispatcher& dispatcher() const {
            return *dispatcher_;
        }

        /**
         * @brief Returns the undo and redo history stack.
         *
         * @return Mutable history reference.
         */
        [[nodiscard]] HistoryStack& history() {
            return *history_;
        }

        /**
         * @brief Returns the undo and redo history stack.
         *
         * @return Read-only history reference.
         */
        [[nodiscard]] const HistoryStack& history() const {
            return *history_;
        }

        /**
         * @brief Executes and stores an undoable editor command.
         *
         * The command is routed through HistoryStack, which delegates command
         * execution to CommandDispatcher and stores successful undoable
         * commands.
         *
         * @param command Owned command to execute.
         * @return Command execution result.
         */
        CommandResult execute_command(
            std::unique_ptr<ICommand> command);

        /**
         * @brief Marks editor subsystems as dirty.
         *
         * Persistent document mutations should normally report dirty flags
         * through command execution. Direct marking remains useful for actions
         * that change transient or non-command editor state.
         *
         * @param flags Dirty flags to add.
         */
        void mark_dirty(EditorDirtyFlags flags) {
            editor_->mark_dirty(flags);
        }

    private:
        Editor* editor_ = nullptr;
        CommandDispatcher* dispatcher_ = nullptr;
        HistoryStack* history_ = nullptr;
    };

} // namespace locus::editor