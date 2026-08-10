/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/window/ApplicationWindow.h"

#include "application/input/InputState.h"
#include "graphics/common/GraphicsConfig.h"

#include <GLFW/glfw3.h>

#include <cmath>
#include <optional>
#include <string>

namespace locus::application {

    namespace {

        [[nodiscard]] ApplicationResult<void> invalid_configuration(
            std::string message)
        {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidConfiguration,
                std::move(message));
        }

        [[nodiscard]] ApplicationResult<void> initialization_failure(
            const char* operation,
            const graphics::GraphicsError& error)
        {
            std::string message = operation;

            if (!error.message.empty()) {
                message += ": ";
                message += error.message;
            }

            return ApplicationError::make(
                ApplicationErrorCode::InitializationFailed,
                std::move(message));
        }

        [[nodiscard]] MouseButton to_mouse_button(int button) noexcept
        {
            switch (button) {
            case GLFW_MOUSE_BUTTON_LEFT:
                return MouseButton::Left;
            case GLFW_MOUSE_BUTTON_RIGHT:
                return MouseButton::Right;
            case GLFW_MOUSE_BUTTON_MIDDLE:
                return MouseButton::Middle;
            case GLFW_MOUSE_BUTTON_4:
                return MouseButton::Button4;
            case GLFW_MOUSE_BUTTON_5:
                return MouseButton::Button5;
            case GLFW_MOUSE_BUTTON_6:
                return MouseButton::Button6;
            case GLFW_MOUSE_BUTTON_7:
                return MouseButton::Button7;
            case GLFW_MOUSE_BUTTON_8:
                return MouseButton::Button8;
            default:
                return MouseButton::Unknown;
            }
        }

        [[nodiscard]] Key to_key(int key) noexcept
        {
            switch (key) {
            case GLFW_KEY_A:
                return Key::A;
            case GLFW_KEY_B:
                return Key::B;
            case GLFW_KEY_C:
                return Key::C;
            case GLFW_KEY_D:
                return Key::D;
            case GLFW_KEY_E:
                return Key::E;
            case GLFW_KEY_F:
                return Key::F;
            case GLFW_KEY_G:
                return Key::G;
            case GLFW_KEY_H:
                return Key::H;
            case GLFW_KEY_I:
                return Key::I;
            case GLFW_KEY_J:
                return Key::J;
            case GLFW_KEY_K:
                return Key::K;
            case GLFW_KEY_L:
                return Key::L;
            case GLFW_KEY_M:
                return Key::M;
            case GLFW_KEY_N:
                return Key::N;
            case GLFW_KEY_O:
                return Key::O;
            case GLFW_KEY_P:
                return Key::P;
            case GLFW_KEY_Q:
                return Key::Q;
            case GLFW_KEY_R:
                return Key::R;
            case GLFW_KEY_S:
                return Key::S;
            case GLFW_KEY_T:
                return Key::T;
            case GLFW_KEY_U:
                return Key::U;
            case GLFW_KEY_V:
                return Key::V;
            case GLFW_KEY_W:
                return Key::W;
            case GLFW_KEY_X:
                return Key::X;
            case GLFW_KEY_Y:
                return Key::Y;
            case GLFW_KEY_Z:
                return Key::Z;
            case GLFW_KEY_0:
                return Key::Num0;
            case GLFW_KEY_1:
                return Key::Num1;
            case GLFW_KEY_2:
                return Key::Num2;
            case GLFW_KEY_3:
                return Key::Num3;
            case GLFW_KEY_4:
                return Key::Num4;
            case GLFW_KEY_5:
                return Key::Num5;
            case GLFW_KEY_6:
                return Key::Num6;
            case GLFW_KEY_7:
                return Key::Num7;
            case GLFW_KEY_8:
                return Key::Num8;
            case GLFW_KEY_9:
                return Key::Num9;
            case GLFW_KEY_ESCAPE:
                return Key::Escape;
            case GLFW_KEY_DELETE:
                return Key::Delete;
            case GLFW_KEY_BACKSPACE:
                return Key::Backspace;
            case GLFW_KEY_ENTER:
                return Key::Enter;
            case GLFW_KEY_SPACE:
                return Key::Space;
            case GLFW_KEY_EQUAL:
            case GLFW_KEY_KP_ADD:
                return Key::Equal;
            case GLFW_KEY_MINUS:
            case GLFW_KEY_KP_SUBTRACT:
                return Key::Minus;
            default:
                return Key::Unknown;
            }
        }

        [[nodiscard]] InputModifiers to_input_modifiers(int mods) noexcept
        {
            InputModifiers result = InputModifiers::None;

            if ((mods & GLFW_MOD_SHIFT) != 0) {
                result |= InputModifiers::Shift;
            }
            if ((mods & GLFW_MOD_CONTROL) != 0) {
                result |= InputModifiers::Control;
            }
            if ((mods & GLFW_MOD_ALT) != 0) {
                result |= InputModifiers::Alt;
            }
            if ((mods & GLFW_MOD_SUPER) != 0) {
                result |= InputModifiers::Super;
            }
            if ((mods & GLFW_MOD_CAPS_LOCK) != 0) {
                result |= InputModifiers::CapsLock;
            }
            if ((mods & GLFW_MOD_NUM_LOCK) != 0) {
                result |= InputModifiers::NumLock;
            }

            return result;
        }

        [[nodiscard]] std::optional<InputEventType>
        mouse_event_type(int action) noexcept
        {
            if (action == GLFW_PRESS) {
                return InputEventType::MouseButtonPressed;
            }
            if (action == GLFW_RELEASE) {
                return InputEventType::MouseButtonReleased;
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<InputEventType>
        key_event_type(int action) noexcept
        {
            switch (action) {
            case GLFW_PRESS:
                return InputEventType::KeyPressed;
            case GLFW_RELEASE:
                return InputEventType::KeyReleased;
            case GLFW_REPEAT:
                return InputEventType::KeyRepeated;
            default:
                return std::nullopt;
            }
        }

    } // namespace

    ApplicationWindow::~ApplicationWindow()
    {
        shutdown();
    }

    ApplicationResult<void> ApplicationWindow::initialize(
        const ApplicationConfig& config)
    {
        if (initialized_) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "ApplicationWindow is already initialized.");
        }

        if (config.title.empty()) {
            return invalid_configuration(
                "Application window title cannot be empty.");
        }

        if (config.initialWidth <= 0 || config.initialHeight <= 0) {
            return invalid_configuration(
                "Application window dimensions must be positive.");
        }

        if (!std::isfinite(config.maximumFrameDeltaSeconds)
            || config.maximumFrameDeltaSeconds < 0.0) {
            return invalid_configuration(
                "Maximum frame delta must be finite and non-negative.");
        }

        graphics::WindowCreateInfo windowInfo{};
        windowInfo.width = config.initialWidth;
        windowInfo.height = config.initialHeight;
        windowInfo.title = config.title;
        windowInfo.decorated = config.decorated;
        windowInfo.maximized = config.startMaximized;

        const graphics::GraphicsResult<void> windowResult =
            window_.create(windowInfo);

        if (!windowResult) {
            return initialization_failure(
                "Failed to create the application window",
                windowResult.error());
        }

        graphics::GraphicsConfig graphicsConfig{};
        graphicsConfig.enableVSync = config.enableVSync;

        const graphics::GraphicsResult<void> contextResult =
            context_.initialize(window_, graphicsConfig);

        if (!contextResult) {
            window_.destroy();
            return initialization_failure(
                "Failed to initialize the application graphics context",
                contextResult.error());
        }

        configuration_ = config;
        initialized_ = true;
        return {};
    }

    void ApplicationWindow::shutdown()
    {
        if (!initialized_) {
            return;
        }

        disconnect_input();
        context_.shutdown();
        window_.destroy();
        initialized_ = false;
    }

    bool ApplicationWindow::initialized() const noexcept
    {
        return initialized_;
    }

    void ApplicationWindow::connect_input(InputState& inputState)
    {
        inputState_ = &inputState;
        inputState_->reset();

        const graphics::CursorPosition cursor = window_.cursor_position();
        inputState_->initialize_cursor({ cursor.x, cursor.y });

        window_.set_cursor_move_callback(
            [this](const graphics::CursorMoveEvent& event) {
                if (!inputState_) {
                    return;
                }

                InputEvent inputEvent{};
                inputEvent.type = InputEventType::CursorMoved;
                inputEvent.cursorPosition = { event.x, event.y };
                inputState_->consume(inputEvent);
            });

        window_.set_mouse_button_callback(
            [this](const graphics::MouseButtonEvent& event) {
                if (!inputState_) {
                    return;
                }

                const auto type = mouse_event_type(event.action);
                if (!type) {
                    return;
                }

                InputEvent inputEvent{};
                inputEvent.type = *type;
                inputEvent.mouseButton = to_mouse_button(event.button);
                inputEvent.modifiers = to_input_modifiers(event.mods);
                inputState_->consume(inputEvent);
            });

        window_.set_scroll_callback(
            [this](const graphics::ScrollEvent& event) {
                if (!inputState_) {
                    return;
                }

                InputEvent inputEvent{};
                inputEvent.type = InputEventType::Scrolled;
                inputEvent.scrollDelta = {
                    event.xOffset,
                    event.yOffset
                };
                inputState_->consume(inputEvent);
            });

        window_.set_key_callback(
            [this](const graphics::KeyEvent& event) {
                if (!inputState_) {
                    return;
                }

                const auto type = key_event_type(event.action);
                if (!type) {
                    return;
                }

                InputEvent inputEvent{};
                inputEvent.type = *type;
                inputEvent.key = to_key(event.key);
                inputEvent.scancode = event.scancode;
                inputEvent.modifiers = to_input_modifiers(event.mods);
                inputState_->consume(inputEvent);
            });

        window_.set_focus_callback(
            [this](const graphics::WindowFocusEvent& event) {
                if (!inputState_) {
                    return;
                }

                InputEvent inputEvent{};
                inputEvent.type = event.focused
                    ? InputEventType::FocusGained
                    : InputEventType::FocusLost;
                inputState_->consume(inputEvent);
            });
    }

    void ApplicationWindow::disconnect_input()
    {
        window_.set_cursor_move_callback({});
        window_.set_mouse_button_callback({});
        window_.set_scroll_callback({});
        window_.set_key_callback({});
        window_.set_focus_callback({});
        inputState_ = nullptr;
    }

    void ApplicationWindow::process_events() const
    {
        if (initialized_) {
            window_.poll_events();
        }
    }

    void ApplicationWindow::present()
    {
        if (initialized_) {
            context_.swap_buffers();
        }
    }

    bool ApplicationWindow::should_close() const
    {
        return !initialized_ || window_.should_close();
    }

    void ApplicationWindow::request_close()
    {
        if (initialized_) {
            window_.request_close();
        }
    }

    std::int32_t ApplicationWindow::width() const noexcept
    {
        return window_.width();
    }

    std::int32_t ApplicationWindow::height() const noexcept
    {
        return window_.height();
    }

    std::int32_t ApplicationWindow::framebuffer_width() const noexcept
    {
        return window_.framebuffer_width();
    }

    std::int32_t ApplicationWindow::framebuffer_height() const noexcept
    {
        return window_.framebuffer_height();
    }

    void ApplicationWindow::set_clipboard_text(const std::string& text)
    {
        if (initialized_) {
            window_.set_clipboard_text(text);
        }
    }

    std::string ApplicationWindow::clipboard_text() const
    {
        return initialized_ ? window_.clipboard_text() : std::string{};
    }

    const ApplicationConfig& ApplicationWindow::configuration() const noexcept
    {
        return configuration_;
    }

} // namespace locus::application
