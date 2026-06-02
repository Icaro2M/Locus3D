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

    struct WindowCreateInfo
    {
        i32 width = 1280;
        i32 height = 720;
        std::string title = "Locus3D";

        bool resizable = true;
        bool visible = true;
        bool decorated = true;
        bool maximized = false;

        bool requestOpenGLContext = true;
    };

    class Window
    {
    public:
        using ResizeCallback = std::function<void(const WindowResizeEvent&)>;
        using FramebufferResizeCallback = std::function<void(const FramebufferResizeEvent&)>;
        using FocusCallback = std::function<void(const WindowFocusEvent&)>;
        using CloseCallback = std::function<void(const WindowCloseEvent&)>;
        using CursorMoveCallback = std::function<void(const CursorMoveEvent&)>;
        using MouseButtonCallback = std::function<void(const MouseButtonEvent&)>;
        using ScrollCallback = std::function<void(const ScrollEvent&)>;
        using KeyCallback = std::function<void(const KeyEvent&)>;

    public:
        Window() = default;
        ~Window();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        Window(Window&& other) noexcept;
        Window& operator=(Window&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(const WindowCreateInfo& createInfo);
        void destroy();

        void poll_events() const;
        void swap_buffers() const;

        [[nodiscard]] bool should_close() const;
        void request_close();

        void show();
        void hide();

        void set_title(const std::string& title);
        void set_vsync(bool enabled);

        [[nodiscard]] i32 width() const;
        [[nodiscard]] i32 height() const;

        [[nodiscard]] i32 framebuffer_width() const;
        [[nodiscard]] i32 framebuffer_height() const;

        [[nodiscard]] CursorPosition cursor_position() const;
        void set_cursor_position(double x, double y);

        void set_cursor_mode(CursorMode mode);
        void set_cursor_shape(CursorShape shape);

        void set_resize_callback(ResizeCallback callback);
        void set_framebuffer_resize_callback(FramebufferResizeCallback callback);
        void set_focus_callback(FocusCallback callback);
        void set_close_callback(CloseCallback callback);
        void set_cursor_move_callback(CursorMoveCallback callback);
        void set_mouse_button_callback(MouseButtonCallback callback);
        void set_scroll_callback(ScrollCallback callback);
        void set_key_callback(KeyCallback callback);

        [[nodiscard]] GLFWwindow* native_handle();
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