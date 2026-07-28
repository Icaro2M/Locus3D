/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>
#include <type_traits>

namespace locus::application {

    /**
     * @brief Two-dimensional input value in logical window coordinates.
     */
    struct InputVector2 {
        double x = 0.0;
        double y = 0.0;
    };

    /**
     * @brief Mouse buttons recognized by the application input layer.
     */
    enum class MouseButton : std::int32_t {
        Unknown = -1,
        Left = 0,
        Right = 1,
        Middle = 2,
        Button4 = 3,
        Button5 = 4,
        Button6 = 5,
        Button7 = 6,
        Button8 = 7
    };

    /**
     * @brief Backend-independent keys recognized by application input.
     */
    enum class Key : std::int32_t {
        Unknown = 0,
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z,
        Num0,
        Num1,
        Num2,
        Num3,
        Num4,
        Num5,
        Num6,
        Num7,
        Num8,
        Num9,
        Escape,
        Delete,
        Backspace,
        Enter,
        Space,
        Equal,
        Minus
    };

    /**
     * @brief Modifier keys active for an input event.
     */
    enum class InputModifiers : std::uint32_t {
        None = 0,
        Shift = 1u << 0,
        Control = 1u << 1,
        Alt = 1u << 2,
        Super = 1u << 3,
        CapsLock = 1u << 4,
        NumLock = 1u << 5
    };

    [[nodiscard]] constexpr InputModifiers operator|(
        InputModifiers lhs,
        InputModifiers rhs) noexcept
    {
        using Value = std::underlying_type_t<InputModifiers>;
        return static_cast<InputModifiers>(
            static_cast<Value>(lhs) | static_cast<Value>(rhs));
    }

    constexpr InputModifiers& operator|=(
        InputModifiers& lhs,
        InputModifiers rhs) noexcept
    {
        lhs = lhs | rhs;
        return lhs;
    }

    [[nodiscard]] constexpr InputModifiers operator&(
        InputModifiers lhs,
        InputModifiers rhs) noexcept
    {
        using Value = std::underlying_type_t<InputModifiers>;
        return static_cast<InputModifiers>(
            static_cast<Value>(lhs) & static_cast<Value>(rhs));
    }

    [[nodiscard]] constexpr bool has_input_modifier(
        InputModifiers modifiers,
        InputModifiers modifier) noexcept
    {
        return (modifiers & modifier) == modifier;
    }

    /**
     * @brief Event categories consumed by InputState.
     */
    enum class InputEventType {
        CursorMoved,
        MouseButtonPressed,
        MouseButtonReleased,
        KeyPressed,
        KeyReleased,
        KeyRepeated,
        Scrolled,
        FocusGained,
        FocusLost
    };

    /**
     * @brief Backend-independent event delivered to the application input state.
     */
    struct InputEvent {
        InputEventType type = InputEventType::CursorMoved;
        InputVector2 cursorPosition{};
        InputVector2 scrollDelta{};
        MouseButton mouseButton = MouseButton::Unknown;
        Key key = Key::Unknown;
        std::int32_t scancode = 0;
        InputModifiers modifiers = InputModifiers::None;
    };

} // namespace locus::application
