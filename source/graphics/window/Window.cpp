/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/window/Window.h"

#include "graphics/common/GraphicsError.h"

#include <GLFW/glfw3.h>

#include <utility>

namespace locus::graphics
{

    namespace
    {
        /*
         * GLFW uses integer constants for boolean hints, while the graphics
         * interface keeps the user-facing configuration as bool values.
         */
        int to_glfw_bool(bool value)
        {
            return value ? GLFW_TRUE : GLFW_FALSE;
        }

        int to_glfw_cursor_mode(CursorMode mode)
        {
            switch (mode)
            {
            case CursorMode::Normal:
                return GLFW_CURSOR_NORMAL;
            case CursorMode::Hidden:
                return GLFW_CURSOR_HIDDEN;
            case CursorMode::Disabled:
                return GLFW_CURSOR_DISABLED;
            }

            return GLFW_CURSOR_NORMAL;
        }

        int to_glfw_cursor_shape(CursorShape shape)
        {
            switch (shape)
            {
            case CursorShape::Arrow:
                return GLFW_ARROW_CURSOR;
            case CursorShape::IBeam:
                return GLFW_IBEAM_CURSOR;
            case CursorShape::Crosshair:
                return GLFW_CROSSHAIR_CURSOR;
            case CursorShape::Hand:
                return GLFW_HAND_CURSOR;
            case CursorShape::HorizontalResize:
                return GLFW_HRESIZE_CURSOR;
            case CursorShape::VerticalResize:
                return GLFW_VRESIZE_CURSOR;
            }

            return GLFW_ARROW_CURSOR;
        }

    } 

    Window::~Window()
    {
        destroy();
    }

    Window::Window(Window&& other) noexcept
    {
        *this = std::move(other);
    }

    Window& Window::operator=(Window&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        /*
         * Transfer the native window pointer and then rebind GLFW's user pointer
         * so static callbacks continue to reach the new C++ owner.
         */
        window_ = other.window_;
        cursor_ = other.cursor_;

        width_ = other.width_;
        height_ = other.height_;
        framebufferWidth_ = other.framebufferWidth_;
        framebufferHeight_ = other.framebufferHeight_;

        resizeCallback_ = std::move(other.resizeCallback_);
        framebufferResizeCallback_ = std::move(other.framebufferResizeCallback_);
        focusCallback_ = std::move(other.focusCallback_);
        closeCallback_ = std::move(other.closeCallback_);
        cursorMoveCallback_ = std::move(other.cursorMoveCallback_);
        mouseButtonCallback_ = std::move(other.mouseButtonCallback_);
        scrollCallback_ = std::move(other.scrollCallback_);
        keyCallback_ = std::move(other.keyCallback_);

        other.window_ = nullptr;
        other.cursor_ = nullptr;
        other.width_ = 0;
        other.height_ = 0;
        other.framebufferWidth_ = 0;
        other.framebufferHeight_ = 0;

        if (window_)
        {
            glfwSetWindowUserPointer(window_, this);
        }

        return *this;
    }

    GraphicsResult<void> Window::create(const WindowCreateInfo& createInfo)
    {
        if (window_)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot create a window because this Window instance already owns one.");
        }

        if (!glfwInit())
        {
            return GraphicsError::make(
                GraphicsErrorCode::WindowCreationFailed,
                "GLFW initialization failed.");
        }

        glfwWindowHint(GLFW_RESIZABLE, to_glfw_bool(createInfo.resizable));
        glfwWindowHint(GLFW_VISIBLE, to_glfw_bool(createInfo.visible));
        glfwWindowHint(GLFW_DECORATED, to_glfw_bool(createInfo.decorated));
        glfwWindowHint(GLFW_MAXIMIZED, to_glfw_bool(createInfo.maximized));

        /*
         * Window creation owns the context hints because GLFW consumes them only
         * while creating the native window.
         */
        if (createInfo.requestOpenGLContext)
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, createInfo.openglMajorVersion);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, createInfo.openglMinorVersion);

            if (createInfo.openglCoreProfile)
            {
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            }

            glfwWindowHint(
                GLFW_OPENGL_FORWARD_COMPAT,
                to_glfw_bool(createInfo.openglForwardCompatible));

            glfwWindowHint(
                GLFW_OPENGL_DEBUG_CONTEXT,
                to_glfw_bool(createInfo.openglDebugContext));
        }
        else
        {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }

        window_ = glfwCreateWindow(
            createInfo.width,
            createInfo.height,
            createInfo.title.c_str(),
            nullptr,
            nullptr);

        if (!window_)
        {
            glfwTerminate();

            return GraphicsError::make(
                GraphicsErrorCode::WindowCreationFailed,
                "GLFW window creation failed.");
        }

        width_ = createInfo.width;
        height_ = createInfo.height;

        glfwGetFramebufferSize(window_, &framebufferWidth_, &framebufferHeight_);

        glfwSetWindowUserPointer(window_, this);
        install_callbacks();

        return {};
    }

    void Window::destroy()
    {
        release_cursor();

        /*
         * GLFW is currently initialized per Window instance. This is simple for
         * the initial layer, but should become reference-counted if multi-window
         * support is introduced.
         */
        if (window_)
        {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }

        glfwTerminate();

        width_ = 0;
        height_ = 0;
        framebufferWidth_ = 0;
        framebufferHeight_ = 0;
    }

    void Window::poll_events() const
    {
        glfwPollEvents();
    }

    void Window::swap_buffers() const
    {
        if (window_)
        {
            glfwSwapBuffers(window_);
        }
    }

    bool Window::should_close() const
    {
        if (!window_)
        {
            return true;
        }

        return glfwWindowShouldClose(window_) == GLFW_TRUE;
    }

    void Window::request_close()
    {
        if (window_)
        {
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        }
    }

    void Window::show()
    {
        if (window_)
        {
            glfwShowWindow(window_);
        }
    }

    void Window::hide()
    {
        if (window_)
        {
            glfwHideWindow(window_);
        }
    }

    void Window::set_title(const std::string& title)
    {
        if (window_)
        {
            glfwSetWindowTitle(window_, title.c_str());
        }
    }

    void Window::set_vsync(bool enabled)
    {
        glfwSwapInterval(enabled ? 1 : 0);
    }

    i32 Window::width() const
    {
        return width_;
    }

    i32 Window::height() const
    {
        return height_;
    }

    i32 Window::framebuffer_width() const
    {
        return framebufferWidth_;
    }

    i32 Window::framebuffer_height() const
    {
        return framebufferHeight_;
    }

    CursorPosition Window::cursor_position() const
    {
        CursorPosition position{};

        if (window_)
        {
            glfwGetCursorPos(window_, &position.x, &position.y);
        }

        return position;
    }

    void Window::set_cursor_position(double x, double y)
    {
        if (window_)
        {
            glfwSetCursorPos(window_, x, y);
        }
    }

    void Window::set_cursor_mode(CursorMode mode)
    {
        if (window_)
        {
            glfwSetInputMode(window_, GLFW_CURSOR, to_glfw_cursor_mode(mode));
        }
    }

    void Window::set_cursor_shape(CursorShape shape)
    {
        if (!window_)
        {
            return;
        }

        release_cursor();

        cursor_ = glfwCreateStandardCursor(to_glfw_cursor_shape(shape));
        glfwSetCursor(window_, cursor_);
    }

    void Window::set_resize_callback(ResizeCallback callback)
    {
        resizeCallback_ = std::move(callback);
    }

    void Window::set_framebuffer_resize_callback(FramebufferResizeCallback callback)
    {
        framebufferResizeCallback_ = std::move(callback);
    }

    void Window::set_focus_callback(FocusCallback callback)
    {
        focusCallback_ = std::move(callback);
    }

    void Window::set_close_callback(CloseCallback callback)
    {
        closeCallback_ = std::move(callback);
    }

    void Window::set_cursor_move_callback(CursorMoveCallback callback)
    {
        cursorMoveCallback_ = std::move(callback);
    }

    void Window::set_mouse_button_callback(MouseButtonCallback callback)
    {
        mouseButtonCallback_ = std::move(callback);
    }

    void Window::set_scroll_callback(ScrollCallback callback)
    {
        scrollCallback_ = std::move(callback);
    }

    void Window::set_key_callback(KeyCallback callback)
    {
        keyCallback_ = std::move(callback);
    }

    GLFWwindow* Window::native_handle()
    {
        return window_;
    }

    const GLFWwindow* Window::native_handle() const
    {
        return window_;
    }

    void Window::install_callbacks()
    {
        /*
         * GLFW callbacks are static C hooks. Each callback resolves the owning
         * Window through glfwGetWindowUserPointer before dispatching user code.
         */
        glfwSetWindowSizeCallback(window_, &Window::glfw_window_size_callback);
        glfwSetFramebufferSizeCallback(window_, &Window::glfw_framebuffer_size_callback);
        glfwSetWindowFocusCallback(window_, &Window::glfw_window_focus_callback);
        glfwSetWindowCloseCallback(window_, &Window::glfw_window_close_callback);
        glfwSetCursorPosCallback(window_, &Window::glfw_cursor_position_callback);
        glfwSetMouseButtonCallback(window_, &Window::glfw_mouse_button_callback);
        glfwSetScrollCallback(window_, &Window::glfw_scroll_callback);
        glfwSetKeyCallback(window_, &Window::glfw_key_callback);
    }

    void Window::release_cursor()
    {
        if (cursor_)
        {
            glfwDestroyCursor(cursor_);
            cursor_ = nullptr;
        }
    }

    void Window::glfw_window_size_callback(GLFWwindow* window, int width, int height)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (!self)
        {
            return;
        }

        self->width_ = width;
        self->height_ = height;

        if (self->resizeCallback_)
        {
            self->resizeCallback_(WindowResizeEvent{ width, height });
        }
    }

    void Window::glfw_framebuffer_size_callback(GLFWwindow* window, int width, int height)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (!self)
        {
            return;
        }

        self->framebufferWidth_ = width;
        self->framebufferHeight_ = height;

        if (self->framebufferResizeCallback_)
        {
            self->framebufferResizeCallback_(FramebufferResizeEvent{ width, height });
        }
    }

    void Window::glfw_window_focus_callback(GLFWwindow* window, int focused)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (!self)
        {
            return;
        }

        if (self->focusCallback_)
        {
            self->focusCallback_(WindowFocusEvent{ focused == GLFW_TRUE });
        }
    }

    void Window::glfw_window_close_callback(GLFWwindow* window)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (!self)
        {
            return;
        }

        if (self->closeCallback_)
        {
            self->closeCallback_(WindowCloseEvent{});
        }
    }

    void Window::glfw_cursor_position_callback(GLFWwindow* window, double x, double y)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (!self)
        {
            return;
        }

        if (self->cursorMoveCallback_)
        {
            self->cursorMoveCallback_(CursorMoveEvent{ x, y });
        }
    }

    void Window::glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (!self)
        {
            return;
        }

        if (self->mouseButtonCallback_)
        {
            self->mouseButtonCallback_(MouseButtonEvent{ button, action, mods });
        }
    }

    void Window::glfw_scroll_callback(GLFWwindow* window, double xOffset, double yOffset)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (!self)
        {
            return;
        }

        if (self->scrollCallback_)
        {
            self->scrollCallback_(ScrollEvent{ xOffset, yOffset });
        }
    }

    void Window::glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto* self = static_cast<Window*>(glfwGetWindowUserPointer(window));

        if (!self)
        {
            return;
        }

        if (self->keyCallback_)
        {
            self->keyCallback_(KeyEvent{ key, scancode, action, mods });
        }
    }

}
