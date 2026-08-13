/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/history/HistoryStack.h"

#include "editor/command/CommandDispatcher.h"
#include "editor/command/ICommand.h"

#include <utility>

namespace locus::editor {

	CommandResult HistoryStack::execute(CommandDispatcher& dispatcher, std::unique_ptr<ICommand> command)
	{
		if (!command) {
			return CommandResult::fail("Cannot execute a null command.");
		}

		const bool undoable = command->is_undoable();
		CommandResult result = dispatcher.execute(*command);

		if (!result.success) {
			return result;
		}

		clear_redo();

		if (undoable) {
			push_executed(std::move(command), result);
		}

		return result;
	}

	bool HistoryStack::push_executed(std::unique_ptr<ICommand> command, CommandResult result)
	{
		if (!command || !result.success || !command->is_undoable()) {
			return false;
		}

        const StateId beforeState = currentState_;
        const StateId afterState = allocate_state();
        currentState_ = afterState;

		undoStack_.emplace_back(
            std::move(command),
            std::move(result),
            beforeState,
            afterState);
		trim_undo_to_limit();
		clear_redo();

		return true;
	}

	CommandResult HistoryStack::undo(CommandDispatcher& dispatcher)
	{
		if (!can_undo()) {
			return CommandResult::fail("Nothing to undo.");
		}

		HistoryEntry entry = std::move(undoStack_.back());
		undoStack_.pop_back();

		CommandResult result = dispatcher.undo(entry.command());
		entry.set_last_result(result);

		if (result.success) {
            currentState_ = entry.before_state();
			redoStack_.push_back(std::move(entry));
		}
		else {
			undoStack_.push_back(std::move(entry));
		}

		return result;
	}

	CommandResult HistoryStack::redo(CommandDispatcher& dispatcher)
	{
		if (!can_redo()) {
			return CommandResult::fail("Nothing to redo.");
		}

		HistoryEntry entry = std::move(redoStack_.back());
		redoStack_.pop_back();

		CommandResult result = dispatcher.redo(entry.command());
		entry.set_last_result(result);

		if (result.success) {
            currentState_ = entry.after_state();
			undoStack_.push_back(std::move(entry));
			trim_undo_to_limit();
		}
		else {
			redoStack_.push_back(std::move(entry));
		}

		return result;
	}

	bool HistoryStack::can_undo() const
	{
		return !undoStack_.empty();
	}

	bool HistoryStack::can_redo() const
	{
		return !redoStack_.empty();
	}

	std::string HistoryStack::undo_name() const
	{
		if (!can_undo()) {
			return {};
		}

		return undoStack_.back().command_name();
	}

	std::string HistoryStack::redo_name() const
	{
		if (!can_redo()) {
			return {};
		}

		return redoStack_.back().command_name();
	}

	void HistoryStack::clear()
	{
		undoStack_.clear();
		redoStack_.clear();
        currentState_ = allocate_state();
        cleanState_ = currentState_;
	}

	void HistoryStack::clear_redo()
	{
		redoStack_.clear();
	}

	void HistoryStack::set_max_entries(std::size_t maxEntries)
	{
		maxEntries_ = maxEntries;
		trim_undo_to_limit();
	}

	std::size_t HistoryStack::max_entries() const
	{
		return maxEntries_;
	}

	std::size_t HistoryStack::undo_size() const
	{
		return undoStack_.size();
	}

	std::size_t HistoryStack::redo_size() const
	{
		return redoStack_.size();
	}

	bool HistoryStack::empty() const
	{
		return undoStack_.empty() && redoStack_.empty();
	}

    void HistoryStack::mark_clean() noexcept
    {
        cleanState_ = currentState_;
    }

    bool HistoryStack::is_clean() const noexcept
    {
        return currentState_ == cleanState_;
    }

    HistoryStack::StateId HistoryStack::current_state() const noexcept
    {
        return currentState_;
    }

	void HistoryStack::trim_undo_to_limit()
	{
		if (maxEntries_ == 0u) {
			return;
		}

		while (undoStack_.size() > maxEntries_) {
            if (undoStack_.front().before_state() == cleanState_
                || undoStack_.front().after_state() == cleanState_) {
                cleanState_ = 0u;
            }
			undoStack_.erase(undoStack_.begin());
		}
	}

    HistoryStack::StateId HistoryStack::allocate_state() noexcept
    {
        return nextState_++;
    }

}
