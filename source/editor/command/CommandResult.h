/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/EditorTypes.h"

#include <string>

namespace locus::editor {

	/**
	 * @brief Result returned by editor command execution.
	 */
	struct CommandResult {
		/**
		 * @brief True when the command completed successfully.
		 */
		bool success = true;

		/**
		 * @brief Dirty editor subsystems produced by the command.
		 */
		EditorDirtyFlags dirtyFlags = EditorDirtyFlags::None;

		/**
		 * @brief Optional diagnostic message.
		 */
		std::string message{};

		/**
		 * @brief Creates a successful command result.
		 *
		 * @param flags Dirty flags produced by the command.
		 * @param message Optional diagnostic message.
		 * @return Successful command result.
		 */
		[[nodiscard]] static CommandResult ok(
			EditorDirtyFlags flags = EditorDirtyFlags::None,
			std::string message = {})
		{
			CommandResult result{};
			result.success = true;
			result.dirtyFlags = flags;
			result.message = std::move(message);
			return result;
		}

		/**
		 * @brief Creates a failed command result.
		 *
		 * @param message Failure diagnostic message.
		 * @param flags Dirty flags produced before the failure, if any.
		 * @return Failed command result.
		 */
		[[nodiscard]] static CommandResult fail(
			std::string message,
			EditorDirtyFlags flags = EditorDirtyFlags::None)
		{
			CommandResult result{};
			result.success = false;
			result.dirtyFlags = flags;
			result.message = std::move(message);
			return result;
		}

		/**
		 * @brief Checks whether the command succeeded.
		 *
		 * @return True when successful.
		 */
		[[nodiscard]] explicit operator bool() const
		{
			return success;
		}
	};

}