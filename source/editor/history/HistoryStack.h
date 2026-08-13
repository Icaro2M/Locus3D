/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/history/HistoryEntry.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace locus::editor {

	class CommandDispatcher;
	class ICommand;

	/**
	 * @brief Undo and redo stack for editor commands.
	 *
	 * The history stack owns undoable command instances after successful execution.
	 * It does not directly mutate editor state; all command execution is routed
	 * through CommandDispatcher.
	 */
	class HistoryStack {
	public:
        using StateId = HistoryEntry::StateId;
		/**
		 * @brief Creates an empty history stack.
		 */
		HistoryStack() = default;

		HistoryStack(const HistoryStack&) = delete;
		HistoryStack& operator=(const HistoryStack&) = delete;
		HistoryStack(HistoryStack&&) noexcept = default;
		HistoryStack& operator=(HistoryStack&&) noexcept = default;

		/**
		 * @brief Destroys the history stack.
		 */
		~HistoryStack() = default;

		/**
		 * @brief Executes a command and stores it when it is undoable.
		 *
		 * @param dispatcher Dispatcher used to execute the command.
		 * @param command Command to execute.
		 * @return Execution result.
		 */
		CommandResult execute(CommandDispatcher& dispatcher, std::unique_ptr<ICommand> command);

		/**
		 * @brief Stores an already executed undoable command.
		 *
		 * @param command Command to store.
		 * @param result Execution result associated with the command.
		 * @return True when stored.
		 */
		bool push_executed(std::unique_ptr<ICommand> command, CommandResult result);

		/**
		 * @brief Undoes the latest undoable command.
		 *
		 * @param dispatcher Dispatcher used to undo the command.
		 * @return Undo result.
		 */
		CommandResult undo(CommandDispatcher& dispatcher);

		/**
		 * @brief Redoes the latest undone command.
		 *
		 * @param dispatcher Dispatcher used to redo the command.
		 * @return Redo result.
		 */
		CommandResult redo(CommandDispatcher& dispatcher);

		/**
		 * @brief Checks whether undo is currently available.
		 *
		 * @return True when an undo entry exists.
		 */
		[[nodiscard]] bool can_undo() const;

		/**
		 * @brief Checks whether redo is currently available.
		 *
		 * @return True when a redo entry exists.
		 */
		[[nodiscard]] bool can_redo() const;

		/**
		 * @brief Returns the command name that would be undone next.
		 *
		 * @return Undo command name, or an empty string when unavailable.
		 */
		[[nodiscard]] std::string undo_name() const;

		/**
		 * @brief Returns the command name that would be redone next.
		 *
		 * @return Redo command name, or an empty string when unavailable.
		 */
		[[nodiscard]] std::string redo_name() const;

		/**
		 * @brief Clears undo and redo entries.
		 */
		void clear();

		/**
		 * @brief Clears only redo entries.
		 */
		void clear_redo();

		/**
		 * @brief Sets the maximum number of undo entries retained.
		 *
		 * A value of 0 means unlimited history.
		 *
		 * @param maxEntries Maximum undo entry count.
		 */
		void set_max_entries(std::size_t maxEntries);

		/**
		 * @brief Returns the maximum number of undo entries retained.
		 *
		 * @return Maximum entry count, or 0 for unlimited.
		 */
		[[nodiscard]] std::size_t max_entries() const;

		/**
		 * @brief Returns the number of undo entries.
		 *
		 * @return Undo entry count.
		 */
		[[nodiscard]] std::size_t undo_size() const;

		/**
		 * @brief Returns the number of redo entries.
		 *
		 * @return Redo entry count.
		 */
		[[nodiscard]] std::size_t redo_size() const;

		/**
		 * @brief Checks whether both undo and redo stacks are empty.
		 *
		 * @return True when no history entries exist.
		 */
		[[nodiscard]] bool empty() const;

        /**
         * @brief Marks the current history state as the saved checkpoint.
         */
        void mark_clean() noexcept;

        /**
         * @brief Checks whether current history state matches the saved state.
         *
         * @return True when no undoable document edit diverged from save point.
         */
        [[nodiscard]] bool is_clean() const noexcept;

        /**
         * @brief Returns the opaque identity of the current history state.
         */
        [[nodiscard]] StateId current_state() const noexcept;

	private:
		void trim_undo_to_limit();
        [[nodiscard]] StateId allocate_state() noexcept;

		std::vector<HistoryEntry> undoStack_{};
		std::vector<HistoryEntry> redoStack_{};
		std::size_t maxEntries_ = 0u;
        StateId currentState_ = 1u;
        StateId cleanState_ = 1u;
        StateId nextState_ = 2u;
	};

}
