/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/EditorTypes.h"

#include <string>
#include <utility>

namespace locus::editor {

    /**
     * @brief High-level outcome of one editor tool operation.
     */
    enum class ToolResultCode {
        /**
         * @brief Tool did not use the request or event.
         */
        Ignored,

        /**
         * @brief Tool consumed the request without changing lifecycle state.
         */
        Consumed,

        /**
         * @brief Tool started a new interaction.
         */
        Started,

        /**
         * @brief Tool updated an active interaction.
         */
        Updated,

        /**
         * @brief Tool confirmed and completed an interaction.
         */
        Confirmed,

        /**
         * @brief Tool cancelled an interaction.
         */
        Cancelled,

        /**
         * @brief Tool could not complete the requested operation.
         */
        Failed
    };

    /**
     * @brief Result returned by editor tool lifecycle and event operations.
     */
    struct ToolResult {
        /**
         * @brief High-level result code.
         */
        ToolResultCode code = ToolResultCode::Ignored;

        /**
         * @brief Dirty editor subsystems produced by the operation.
         */
        EditorDirtyFlags dirtyFlags = EditorDirtyFlags::None;

        /**
         * @brief Optional diagnostic message.
         */
        std::string message{};

        /**
         * @brief Creates an ignored result.
         *
         * @return Ignored result.
         */
        [[nodiscard]] static ToolResult ignored() {
            return {};
        }

        /**
         * @brief Creates a consumed result.
         *
         * @param flags Dirty flags produced by the operation.
         * @param message Optional diagnostic message.
         * @return Consumed result.
         */
        [[nodiscard]] static ToolResult consumed(
            EditorDirtyFlags flags = EditorDirtyFlags::None,
            std::string message = {}) {

            return make(
                ToolResultCode::Consumed,
                flags,
                std::move(message));
        }

        /**
         * @brief Creates an interaction-started result.
         *
         * @param flags Dirty flags produced by the operation.
         * @param message Optional diagnostic message.
         * @return Started result.
         */
        [[nodiscard]] static ToolResult started(
            EditorDirtyFlags flags = EditorDirtyFlags::None,
            std::string message = {}) {

            return make(
                ToolResultCode::Started,
                flags,
                std::move(message));
        }

        /**
         * @brief Creates an interaction-updated result.
         *
         * @param flags Dirty flags produced by the operation.
         * @param message Optional diagnostic message.
         * @return Updated result.
         */
        [[nodiscard]] static ToolResult updated(
            EditorDirtyFlags flags = EditorDirtyFlags::None,
            std::string message = {}) {

            return make(
                ToolResultCode::Updated,
                flags,
                std::move(message));
        }

        /**
         * @brief Creates an interaction-confirmed result.
         *
         * @param flags Dirty flags produced by the operation.
         * @param message Optional diagnostic message.
         * @return Confirmed result.
         */
        [[nodiscard]] static ToolResult confirmed(
            EditorDirtyFlags flags = EditorDirtyFlags::None,
            std::string message = {}) {

            return make(
                ToolResultCode::Confirmed,
                flags,
                std::move(message));
        }

        /**
         * @brief Creates an interaction-cancelled result.
         *
         * @param flags Dirty flags produced by cancellation.
         * @param message Optional diagnostic message.
         * @return Cancelled result.
         */
        [[nodiscard]] static ToolResult cancelled(
            EditorDirtyFlags flags = EditorDirtyFlags::None,
            std::string message = {}) {

            return make(
                ToolResultCode::Cancelled,
                flags,
                std::move(message));
        }

        /**
         * @brief Creates a failed result.
         *
         * @param message Failure diagnostic message.
         * @param flags Dirty flags produced before failure, if any.
         * @return Failed result.
         */
        [[nodiscard]] static ToolResult fail(
            std::string message,
            EditorDirtyFlags flags = EditorDirtyFlags::None) {

            return make(
                ToolResultCode::Failed,
                flags,
                std::move(message));
        }

        /**
         * @brief Checks whether the operation failed.
         *
         * @return True when the result code is Failed.
         */
        [[nodiscard]] bool failed() const {
            return code == ToolResultCode::Failed;
        }

        /**
         * @brief Checks whether the request was consumed by the tool.
         *
         * Ignored requests are not consumed. Every other result code indicates that
         * the tool handled the request, including failures and cancellations.
         *
         * @return True when the request was handled.
         */
        [[nodiscard]] bool consumed() const {
            return code != ToolResultCode::Ignored;
        }

        /**
         * @brief Checks whether an interaction reached a terminal state.
         *
         * @return True for confirmed, cancelled, or failed results.
         */
        [[nodiscard]] bool is_terminal() const {
            return
                code == ToolResultCode::Confirmed ||
                code == ToolResultCode::Cancelled ||
                code == ToolResultCode::Failed;
        }

        /**
         * @brief Checks whether the operation completed without failure.
         *
         * @return True for every result other than Failed.
         */
        [[nodiscard]] explicit operator bool() const {
            return !failed();
        }

    private:
        /**
         * @brief Creates a result with the provided values.
         *
         * @param code Result code.
         * @param flags Dirty flags.
         * @param message Diagnostic message.
         * @return Constructed result.
         */
        [[nodiscard]] static ToolResult make(
            ToolResultCode code,
            EditorDirtyFlags flags,
            std::string message) {

            ToolResult result{};
            result.code = code;
            result.dirtyFlags = flags;
            result.message = std::move(message);
            return result;
        }
    };

} // namespace locus::editor