/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/EditorTypes.h"
#include "editor/command/CommandResult.h"

#include <string>
#include <utility>

namespace locus::editor {

    /**
     * @brief High-level outcome of one editor action execution.
     */
    enum class ActionResultCode {
        /**
         * @brief Action completed successfully.
         */
        Executed,

        /**
         * @brief Action is not available in the current editor context.
         */
        Unavailable,

        /**
         * @brief Action was available but could not complete successfully.
         */
        Failed
    };

    /**
     * @brief Result returned by immediate editor action execution.
     */
    struct ActionResult {
        /**
         * @brief High-level execution result code.
         */
        ActionResultCode code = ActionResultCode::Executed;

        /**
         * @brief Dirty editor subsystems produced by the action.
         */
        EditorDirtyFlags dirtyFlags = EditorDirtyFlags::None;

        /**
         * @brief Optional diagnostic message.
         */
        std::string message{};

        /**
         * @brief Creates a successful action result.
         *
         * @param flags Dirty flags produced by the action.
         * @param message Optional diagnostic message.
         * @return Successful result.
         */
        [[nodiscard]] static ActionResult executed(
            EditorDirtyFlags flags = EditorDirtyFlags::None,
            std::string message = {}) {
            return make(
                ActionResultCode::Executed,
                flags,
                std::move(message));
        }

        /**
         * @brief Creates an unavailable action result.
         *
         * @param message Diagnostic message describing why the action is
         * unavailable.
         * @return Unavailable result.
         */
        [[nodiscard]] static ActionResult unavailable(
            std::string message = {}) {
            return make(
                ActionResultCode::Unavailable,
                EditorDirtyFlags::None,
                std::move(message));
        }

        /**
         * @brief Creates a failed action result.
         *
         * @param message Failure diagnostic message.
         * @param flags Dirty flags produced before the failure, if any.
         * @return Failed result.
         */
        [[nodiscard]] static ActionResult fail(
            std::string message,
            EditorDirtyFlags flags = EditorDirtyFlags::None) {
            return make(
                ActionResultCode::Failed,
                flags,
                std::move(message));
        }

        /**
         * @brief Converts a command execution result into an action result.
         *
         * @param result Command result to convert.
         * @return Successful or failed action result.
         */
        [[nodiscard]] static ActionResult from_command(
            CommandResult result) {
            if (!result.success) {
                return fail(
                    std::move(result.message),
                    result.dirtyFlags);
            }

            return executed(
                result.dirtyFlags,
                std::move(result.message));
        }

        /**
         * @brief Checks whether the action completed successfully.
         *
         * @return True when the action was executed.
         */
        [[nodiscard]] bool succeeded() const {
            return code == ActionResultCode::Executed;
        }

        /**
         * @brief Checks whether the action is unavailable.
         *
         * @return True when execution was rejected by the current context.
         */
        [[nodiscard]] bool is_unavailable() const {
            return code == ActionResultCode::Unavailable;
        }

        /**
         * @brief Checks whether action execution failed.
         *
         * @return True when the action failed after being requested.
         */
        [[nodiscard]] bool failed() const {
            return code == ActionResultCode::Failed;
        }

        /**
         * @brief Checks whether the result represents successful execution.
         *
         * @return True when the action was executed successfully.
         */
        [[nodiscard]] explicit operator bool() const {
            return succeeded();
        }

    private:
        /**
         * @brief Creates an action result with the supplied values.
         *
         * @param code High-level result code.
         * @param flags Dirty editor subsystems.
         * @param message Optional diagnostic message.
         * @return Constructed result.
         */
        [[nodiscard]] static ActionResult make(
            ActionResultCode code,
            EditorDirtyFlags flags,
            std::string message) {
            ActionResult result{};
            result.code = code;
            result.dirtyFlags = flags;
            result.message = std::move(message);
            return result;
        }
    };

} // namespace locus::editor