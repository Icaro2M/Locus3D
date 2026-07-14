/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ToolEvent.h"

#include <glm/vec2.hpp>

namespace locus::editor {

    /**
     * @brief Kind of semantic input capture owned by an editor tool.
     *
     * This state does not perform operating-system or window input capture. The
     * application layer remains responsible for physical pointer capture and uses
     * this record only to determine whether the active tool owns the interaction.
     */
    enum class ToolCaptureType {
        /**
         * @brief The tool does not currently own input capture.
         */
        None,

        /**
         * @brief The tool owns an active pointer interaction.
         */
        Pointer
    };

    /**
     * @brief Runtime input capture state of an editor tool.
     */
    struct ToolCapture {
        /**
         * @brief Current capture kind.
         */
        ToolCaptureType type = ToolCaptureType::None;

        /**
         * @brief Pointer button that started the capture.
         */
        ToolPointerButton button = ToolPointerButton::None;

        /**
         * @brief Pointer position at the beginning of the capture.
         */
        glm::vec2 startPosition{ 0.0f, 0.0f };

        /**
         * @brief Most recent pointer position received during capture.
         */
        glm::vec2 currentPosition{ 0.0f, 0.0f };

        /**
         * @brief Checks whether any input is currently captured.
         *
         * @return True when the capture is active.
         */
        [[nodiscard]] bool is_active() const {
            return type != ToolCaptureType::None;
        }

        /**
         * @brief Checks whether pointer input is currently captured.
         *
         * @return True when pointer capture is active.
         */
        [[nodiscard]] bool has_pointer() const {
            return type == ToolCaptureType::Pointer;
        }

        /**
         * @brief Checks whether the captured pointer button matches a button.
         *
         * @param candidate Button to compare.
         * @return True when pointer capture uses the supplied button.
         */
        [[nodiscard]] bool matches_button(
            ToolPointerButton candidate) const {

            return
                has_pointer() &&
                button == candidate;
        }

        /**
         * @brief Returns the total pointer displacement since capture began.
         *
         * @return Pointer displacement in viewport coordinates.
         */
        [[nodiscard]] glm::vec2 total_delta() const {
            return currentPosition - startPosition;
        }

        /**
         * @brief Starts pointer capture.
         *
         * @param capturedButton Pointer button that owns the interaction.
         * @param position Initial viewport position.
         */
        void begin_pointer(
            ToolPointerButton capturedButton,
            const glm::vec2& position) {

            type = ToolCaptureType::Pointer;
            button = capturedButton;
            startPosition = position;
            currentPosition = position;
        }

        /**
         * @brief Updates the current captured pointer position.
         *
         * @param position New viewport position.
         */
        void update_pointer(const glm::vec2& position) {
            if (!has_pointer()) {
                return;
            }

            currentPosition = position;
        }

        /**
         * @brief Clears all capture state.
         */
        void clear() {
            type = ToolCaptureType::None;
            button = ToolPointerButton::None;
            startPosition = glm::vec2{ 0.0f, 0.0f };
            currentPosition = glm::vec2{ 0.0f, 0.0f };
        }
    };

} // namespace locus::editor