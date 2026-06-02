/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/window/Cursor.h"
#include "graphics/window/WindowEvents.h"

#include <functional>
#include <memory>
#include <string>

struct GLFWcursor;
struct GLFWwindow;

namespace locus::graphics
{
    /**
     * @brief Parameters used to create a native application window.
     */
    struct WindowCreateInfo
    {
        /**
         * @brief Initial logical width.
         */
        i32 width = 1280;

        /**
         * @brief Initial logical height.
         */
        i32 height = 720;

        /**
         * @brief Initial window title.
         */
        std::string title = "Locus3D";

        /**
         * @brief True when the user can resize the window.
         */
        bool resizable = true;

        /**
         * @brief True when the window starts visible.
         */
        bool visible = true;

        /**
         * @brief True when the OS window frame should be shown.
         */
        bool decorated = true;

        /**
         * @brief True when the window should start maximized.
         */
        bool maximized = false;

        /**
         * @brief True when GLFW should create an OpenGL context for this window.
         */
        bool requestOpenGLContext = true;

        /**
         * @brief Requested OpenGL major version.
         */
        i32 openglMajorVersion = 4;

        /**
         * @brief Requested OpenGL minor version.
         */
        i32 openglMinorVersion = 5;

        /**
         * @brief True to request an OpenGL core profile context.
         */
        bool openglCoreProfile = true;

        /**
         * @brief True to request a forward-compatible OpenGL context.
         */
        bool openglForwardCompatible = true;

        /**
         * @brief True to request OpenGL debug context support.
         */
        bool openglDebugContext = true;
    };

    /**
     * @brief RAII wrapper for a GLFW window and its input callbacks.
     */
    class Window
    {
    public:
        /**
         * @brief Callback invoked after a logical window resize.
         */
        using ResizeCallback = std::function<void(const WindowResizeEvent&)>;

        /**
         * @brief Callback invoked after a framebuffer resize.
         */
        using FramebufferResizeCallback = std::function<void(const FramebufferResizeEvent&)>;

        /**
         * @brief Callback invoked after a focus state change.
         */
        using FocusCallback = std::function<void(const WindowFocusEvent&)>;

        /**
         * @brief Callback invoked when the window receives a close request.
         */
        using CloseCallback = std::function<void(const WindowCloseEvent&)>;

        /**
         * @brief Callback invoked after cursor movement.
         */
        using CursorMoveCallback = std::function<void(const CursorMoveEvent&)>;

        /**
         * @brief Callback invoked after a mouse button event.
         */
        using MouseButtonCallback = std::function<void(const MouseButtonEvent&)>;

        /**
         * @brief Callback invoked after a scroll event.
         */
        using ScrollCallback = std::function<void(const ScrollEvent&)>;

        /**
         * @brief Callback invoked after a key event.
         */
        using KeyCallback = std::function<void(const KeyEvent&)>;

    public:
        /**
         * @brief Creates an empty window wrapper.
         */
        Window() = default;

        /**
         * @brief Destroys the owned window, if any.
         */
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        /**
         * @brief Creates the native window.
         *
         * @param createInfo Window creation parameters.
         * @return Success or window creation error.
         */
        [[nodiscard]] GraphicsResult<void> create(const WindowCreateInfo& createInfo);

        /**
         * @brief Destroys the native window and resets cached state.
         */
        void destroy();

        /**
         * @brief Polls pending window-system events.
         */
        void poll_events() const;

        /**
         * @brief Swaps the window back and front buffers.
         */
        void swap_buffers() const;

        /**
         * @brief Checks whether the window should close.
         *
         * @return True when the window is closed or has a close request.
         */
        [[nodiscard]] bool should_close() const;

        /**
         * @brief Requests the window to close.
         */
        void request_close();

        /**
         * @brief Shows the window.
         */
        void show();

        /**
         * @brief Hides the window.
         */
        void hide();

        /**
         * @brief Changes the window title.
         *
         * @param title New title text.
         */
        void set_title(const std::string& title);

        /**
         * @brief Enables or disables vsync for the current context.
         *
         * @param enabled True to enable vsync.
         */
        void set_vsync(bool enabled);

        /**
         * @brief Returns the cached logical window width.
         *
         * @return Window width.
         */
        [[nodiscard]] i32 width() const;

        /**
         * @brief Returns the cached logical window height.
         *
         * @return Window height.
         */
        [[nodiscard]] i32 height() const;

        /**
         * @brief Returns the cached framebuffer width.
         *
         * @return Framebuffer width in pixels.
         */
        [[nodiscard]] i32 framebuffer_width() const;

        /**
         * @brief Returns the cached framebuffer height.
         *
         * @return Framebuffer height in pixels.
         */
        [[nodiscard]] i32 framebuffer_height() const;

        /**
         * @brief Returns the current cursor position.
         *
         * @return Cursor position in window coordinates.
         */
        [[nodiscard]] CursorPosition cursor_position() const;

        /**
         * @brief Sets the cursor position.
         *
         * @param x Horizontal cursor coordinate.
         * @param y Vertical cursor coordinate.
         */
        void set_cursor_position(double x, double y);

        /**
         * @brief Sets the cursor visibility and capture mode.
         *
         * @param mode Requested cursor mode.
         */
        void set_cursor_mode(CursorMode mode);

        /**
         * @brief Sets the standard cursor shape.
         *
         * @param shape Requested cursor shape.
         */
        void set_cursor_shape(CursorShape shape);

        /**
         * @brief Sets the window resize callback.
         *
         * @param callback Callback function.
         */
        void set_resize_callback(ResizeCallback callback);

        /**
         * @brief Sets the framebuffer resize callback.
         *
         * @param callback Callback function.
         */
        void set_framebuffer_resize_callback(FramebufferResizeCallback callback);

        /**
         * @brief Sets the focus callback.
         *
         * @param callback Callback function.
         */
        void set_focus_callback(FocusCallback callback);

        /**
         * @brief Sets the close callback.
         *
         * @param callback Callback function.
         */
        void set_close_callback(CloseCallback callback);

        /**
         * @brief Sets the cursor movement callback.
         *
         * @param callback Callback function.
         */
        void set_cursor_move_callback(CursorMoveCallback callback);

        /**
         * @brief Sets the mouse button callback.
         *
         * @param callback Callback function.
         */
        void set_mouse_button_callback(MouseButtonCallback callback);

        /**
         * @brief Sets the scroll callback.
         *
         * @param callback Callback function.
         */
        void set_scroll_callback(ScrollCallback callback);

        /**
         * @brief Sets the keyboard callback.
         *
         * @param callback Callback function.
         */
        void set_key_callback(KeyCallback callback);

        /**
         * @brief Returns the native GLFW window handle.
         *
         * @return Mutable native window pointer.
         */
        [[nodiscard]] GLFWwindow* native_handle();

        /**
         * @brief Returns the native GLFW window handle.
         *
         * @return Read-only native window pointer.
         */
        [[nodiscard]] const GLFWwindow* native_handle() const;

    private:
        static void glfw_window_size_callback(GLFWwindow* window, int width, int height);
        static void glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height);
        static void glfw_window_focus_callback(GLFWwindow* window, int focused);
        static void glfw_window_close_callback(GLFWwindow* window);
        static void glfw_cursor_position_callback(GLFWwindow* window, double x, double y);
        static void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
        static void glfw_scroll_callback(GLFWwindow* window, double xOffset, double yOffset);
        static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

        void install_callbacks();
        void release_cursor();

    private:
        GLFWwindow* window_ = nullptr;
        GLFWcursor* cursor_ = nullptr;

        i32 width_ = 0;
        i32 height_ = 0;
        i32 framebufferWidth_ = 0;
        i32 framebufferHeight_ = 0;

        ResizeCallback resizeCallback_;
        FramebufferResizeCallback framebufferResizeCallback_;
        FocusCallback focusCallback_;
        CloseCallback closeCallback_;
        CursorMoveCallback cursorMoveCallback_;
        MouseButtonCallback mouseButtonCallback_;
        ScrollCallback scrollCallback_;
        KeyCallback keyCallback_;
    };

}
