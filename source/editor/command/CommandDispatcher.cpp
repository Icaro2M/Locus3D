/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/CommandDispatcher.h"

#include "editor/command/ICommand.h"

namespace locus::editor {

	CommandDispatcher::CommandDispatcher(Editor& editor)
		: context_(editor)
	{
	}

	CommandContext& CommandDispatcher::context()
	{
		return context_;
	}

	const CommandContext& CommandDispatcher::context() const
	{
		return context_;
	}

	CommandResult CommandDispatcher::execute(ICommand& command)
	{
		lastResult_ = command.execute(context_);
		apply_result(lastResult_);
		return lastResult_;
	}

	CommandResult CommandDispatcher::execute(std::unique_ptr<ICommand> command)
	{
		if (!command) {
			lastResult_ = CommandResult::fail("Cannot execute a null command.");
			return lastResult_;
		}

		return execute(*command);
	}

	CommandResult CommandDispatcher::undo(ICommand& command)
	{
		if (!command.is_undoable()) {
			lastResult_ = CommandResult::fail("Command is not undoable.");
			return lastResult_;
		}

		lastResult_ = command.undo(context_);
		apply_result(lastResult_);
		return lastResult_;
	}

	CommandResult CommandDispatcher::redo(ICommand& command)
	{
		lastResult_ = command.redo(context_);
		apply_result(lastResult_);
		return lastResult_;
	}

	const CommandResult& CommandDispatcher::last_result() const
	{
		return lastResult_;
	}

	void CommandDispatcher::apply_result(const CommandResult& result)
	{
		if (result.dirtyFlags != EditorDirtyFlags::None) {
			context_.mark_dirty(result.dirtyFlags);
		}
	}

}