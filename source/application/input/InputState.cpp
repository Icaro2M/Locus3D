/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/input/InputState.h"

namespace locus::application {

    void InputState::reset()
    {
        buttons_ = {};
        keysDown_.clear();
        keysPressed_.clear();
        keysReleased_.clear();
        cursorPosition_ = {};
        cursorDelta_ = {};
        scrollDelta_ = {};
        modifiers_ = InputModifiers::None;
        cursorInitialized_ = false;
        focused_ = true;
        focusGained_ = false;
        focusLost_ = false;
        frameActive_ = false;
    }

    void InputState::begin_frame()
    {
        for (ButtonState& button : buttons_) {
            button.pressed = false;
            button.released = false;
        }

        keysPressed_.clear();
        keysReleased_.clear();
        cursorDelta_ = {};
        scrollDelta_ = {};
        focusGained_ = false;
        focusLost_ = false;
        frameActive_ = true;
    }

    void InputState::end_frame() noexcept
    {
        frameActive_ = false;
    }

    void InputState::consume(const InputEvent& event)
    {
        if (event.type == InputEventType::FocusGained) {
            focusGained_ = !focused_;
            focused_ = true;
            cursorDelta_ = {};
            cursorInitialized_ = false;
            return;
        }

        if (event.type == InputEventType::FocusLost) {
            focusLost_ = focused_;
            focused_ = false;
            modifiers_ = InputModifiers::None;
            release_all_held_inputs();
            return;
        }

        if (!focused_) {
            return;
        }

        switch (event.type) {
        case InputEventType::CursorMoved:
            if (cursorInitialized_) {
                cursorDelta_.x +=
                    event.cursorPosition.x - cursorPosition_.x;
                cursorDelta_.y +=
                    event.cursorPosition.y - cursorPosition_.y;
            }

            cursorPosition_ = event.cursorPosition;
            cursorInitialized_ = true;
            break;

        case InputEventType::MouseButtonPressed: {
            const std::size_t index = button_index(event.mouseButton);
            if (index == InvalidButtonIndex) {
                break;
            }

            ButtonState& button = buttons_[index];
            button.pressed = !button.down;
            button.down = true;
            modifiers_ = event.modifiers;
            break;
        }

        case InputEventType::MouseButtonReleased: {
            const std::size_t index = button_index(event.mouseButton);
            if (index == InvalidButtonIndex) {
                break;
            }

            ButtonState& button = buttons_[index];
            button.released = button.down;
            button.down = false;
            modifiers_ = event.modifiers;
            break;
        }

        case InputEventType::KeyPressed:
            if (event.key != Key::Unknown) {
                const bool inserted = keysDown_.insert(event.key).second;
                if (inserted) {
                    keysPressed_.insert(event.key);
                }
            }
            modifiers_ = event.modifiers;
            break;

        case InputEventType::KeyReleased:
            if (event.key != Key::Unknown) {
                if (keysDown_.erase(event.key) != 0) {
                    keysReleased_.insert(event.key);
                }
            }
            modifiers_ = event.modifiers;
            break;

        case InputEventType::KeyRepeated:
            if (event.key != Key::Unknown) {
                keysDown_.insert(event.key);
            }
            modifiers_ = event.modifiers;
            break;

        case InputEventType::Scrolled:
            scrollDelta_.x += event.scrollDelta.x;
            scrollDelta_.y += event.scrollDelta.y;
            break;

        case InputEventType::FocusGained:
        case InputEventType::FocusLost:
            break;
        }
    }

    void InputState::initialize_cursor(InputVector2 position) noexcept
    {
        cursorPosition_ = position;
        cursorDelta_ = {};
        cursorInitialized_ = true;
    }

    bool InputState::frame_active() const noexcept
    {
        return frameActive_;
    }

    bool InputState::focused() const noexcept
    {
        return focused_;
    }

    bool InputState::focus_gained() const noexcept
    {
        return focusGained_;
    }

    bool InputState::focus_lost() const noexcept
    {
        return focusLost_;
    }

    const InputVector2& InputState::cursor_position() const noexcept
    {
        return cursorPosition_;
    }

    const InputVector2& InputState::cursor_delta() const noexcept
    {
        return cursorDelta_;
    }

    const InputVector2& InputState::scroll_delta() const noexcept
    {
        return scrollDelta_;
    }

    bool InputState::button_down(MouseButton button) const noexcept
    {
        const std::size_t index = button_index(button);
        return index != InvalidButtonIndex && buttons_[index].down;
    }

    bool InputState::button_pressed(MouseButton button) const noexcept
    {
        const std::size_t index = button_index(button);
        return index != InvalidButtonIndex && buttons_[index].pressed;
    }

    bool InputState::button_released(MouseButton button) const noexcept
    {
        const std::size_t index = button_index(button);
        return index != InvalidButtonIndex && buttons_[index].released;
    }

    bool InputState::key_down(Key key) const
    {
        return keysDown_.find(key) != keysDown_.end();
    }

    bool InputState::key_pressed(Key key) const
    {
        return keysPressed_.find(key) != keysPressed_.end();
    }

    bool InputState::key_released(Key key) const
    {
        return keysReleased_.find(key) != keysReleased_.end();
    }

    InputModifiers InputState::modifiers() const noexcept
    {
        return modifiers_;
    }

    bool InputState::modifier_down(
        InputModifiers modifier) const noexcept
    {
        return has_input_modifier(modifiers_, modifier);
    }

    std::size_t InputState::button_index(MouseButton button) noexcept
    {
        const auto value = static_cast<std::int32_t>(button);
        if (value < 0
            || value >= static_cast<std::int32_t>(MouseButtonCount)) {
            return InvalidButtonIndex;
        }

        return static_cast<std::size_t>(value);
    }

    void InputState::release_all_held_inputs()
    {
        for (ButtonState& button : buttons_) {
            if (button.down) {
                button.released = true;
            }
            button.down = false;
        }

        keysReleased_.insert(keysDown_.begin(), keysDown_.end());
        keysDown_.clear();
    }

} // namespace locus::application
