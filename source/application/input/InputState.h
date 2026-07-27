/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/input/InputEvent.h"

#include <array>
#include <cstddef>
#include <unordered_set>

namespace locus::application {

    /**
     * @brief Stores persistent and per-frame application input state.
     */
    class InputState {
    public:
        /**
         * @brief Clears all state and restores a focused idle input source.
         */
        void reset();

        /**
         * @brief Clears transient values before polling platform events.
         */
        void begin_frame();

        /**
         * @brief Marks the current input frame complete.
         */
        void end_frame() noexcept;

        /**
         * @brief Applies one platform-independent input event.
         *
         * @param event Event to consume.
         */
        void consume(const InputEvent& event);

        /**
         * @brief Seeds cursor position without producing a delta.
         *
         * @param position Current logical cursor position.
         */
        void initialize_cursor(InputVector2 position) noexcept;

        [[nodiscard]] bool frame_active() const noexcept;
        [[nodiscard]] bool focused() const noexcept;
        [[nodiscard]] bool focus_gained() const noexcept;
        [[nodiscard]] bool focus_lost() const noexcept;

        [[nodiscard]] const InputVector2& cursor_position() const noexcept;
        [[nodiscard]] const InputVector2& cursor_delta() const noexcept;
        [[nodiscard]] const InputVector2& scroll_delta() const noexcept;

        [[nodiscard]] bool button_down(MouseButton button) const noexcept;
        [[nodiscard]] bool button_pressed(MouseButton button) const noexcept;
        [[nodiscard]] bool button_released(MouseButton button) const noexcept;

        [[nodiscard]] bool key_down(KeyCode key) const;
        [[nodiscard]] bool key_pressed(KeyCode key) const;
        [[nodiscard]] bool key_released(KeyCode key) const;

        [[nodiscard]] InputModifiers modifiers() const noexcept;
        [[nodiscard]] bool modifier_down(
            InputModifiers modifier) const noexcept;

    private:
        struct ButtonState {
            bool down = false;
            bool pressed = false;
            bool released = false;
        };

        [[nodiscard]] static std::size_t button_index(
            MouseButton button) noexcept;
        void release_all_held_inputs();

    private:
        static constexpr std::size_t MouseButtonCount = 8;
        static constexpr std::size_t InvalidButtonIndex = MouseButtonCount;

        std::array<ButtonState, MouseButtonCount> buttons_{};
        std::unordered_set<KeyCode> keysDown_{};
        std::unordered_set<KeyCode> keysPressed_{};
        std::unordered_set<KeyCode> keysReleased_{};
        InputVector2 cursorPosition_{};
        InputVector2 cursorDelta_{};
        InputVector2 scrollDelta_{};
        InputModifiers modifiers_ = InputModifiers::None;
        bool cursorInitialized_ = false;
        bool focused_ = true;
        bool focusGained_ = false;
        bool focusLost_ = false;
        bool frameActive_ = false;
    };

} // namespace locus::application
