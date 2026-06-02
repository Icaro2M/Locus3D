/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/context/OpenGLContext.h"

#include "graphics/common/GraphicsError.h"
#include "graphics/window/Window.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>

namespace locus::graphics
{

    namespace
    {
        /*
         * OpenGL debug output can be extremely noisy. Notifications are useful
         * while debugging drivers, but they drown out actual errors in normal logs.
         */
        void APIENTRY opengl_debug_callback(
            unsigned int source,
            unsigned int type,
            unsigned int id,
            unsigned int severity,
            int length,
            const char* message,
            const void* userParam)
        {
            (void)source;
            (void)type;
            (void)id;
            (void)length;
            (void)userParam;

            if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
            {
                return;
            }

            std::cerr << "[OpenGL] " << message << '\n';
        }

        const char* safe_gl_string(const unsigned char* value)
        {
            if (!value)
            {
                return "";
            }

            return reinterpret_cast<const char*>(value);
        }

    } 

    OpenGLContext::~OpenGLContext()
    {
        shutdown();
    }

    GraphicsResult<void> OpenGLContext::initialize(
        Window& window,
        const GraphicsConfig& config)
    {
        if (initialized_)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "OpenGLContext is already initialized.");
        }

        if (!window.native_handle())
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot initialize OpenGLContext with an invalid window.");
        }

        window_ = &window;

        /*
         * GLAD must load function pointers from the context that will use them,
         * so the GLFW context is made current before calling gladLoadGLLoader.
         */
        glfwMakeContextCurrent(window.native_handle());

        auto loadResult = load_opengl_functions();

        if (!loadResult)
        {
            window_ = nullptr;
            return loadResult;
        }

        read_capabilities();

        auto versionResult = validate_version(config);

        if (!versionResult)
        {
            window_ = nullptr;
            return versionResult;
        }

        configure_debug_output(config.enableDebugOutput);
        set_vsync(config.enableVSync);

        initialized_ = true;

        return {};
    }

    void OpenGLContext::shutdown()
    {
        if (!initialized_)
        {
            return;
        }

        clear_current();

        window_ = nullptr;
        initialized_ = false;
    }

    void OpenGLContext::make_current()
    {
        if (window_ && window_->native_handle())
        {
            glfwMakeContextCurrent(window_->native_handle());
        }
    }

    void OpenGLContext::clear_current()
    {
        glfwMakeContextCurrent(nullptr);
    }

    void OpenGLContext::swap_buffers()
    {
        if (window_)
        {
            window_->swap_buffers();
        }
    }

    void OpenGLContext::set_vsync(bool enabled)
    {
        glfwSwapInterval(enabled ? 1 : 0);
    }

    const GraphicsCapabilities& OpenGLContext::capabilities() const
    {
        return capabilities_;
    }

    GraphicsResult<void> OpenGLContext::load_opengl_functions()
    {
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)))
        {
            return GraphicsError::make(
                GraphicsErrorCode::GraphicsApiLoadFailed,
                "Failed to load OpenGL functions with GLAD.");
        }

        return {};
    }

    GraphicsResult<void> OpenGLContext::validate_version(const GraphicsConfig& config) const
    {
        if (capabilities_.majorVersion > config.requestedMajorVersion)
        {
            return {};
        }

        if (capabilities_.majorVersion == config.requestedMajorVersion &&
            capabilities_.minorVersion >= config.requestedMinorVersion)
        {
            return {};
        }

        std::ostringstream stream;
        stream
            << "OpenGL "
            << config.requestedMajorVersion
            << "."
            << config.requestedMinorVersion
            << " is required, but the current context is OpenGL "
            << capabilities_.majorVersion
            << "."
            << capabilities_.minorVersion
            << ".";

        return GraphicsError::make(
            GraphicsErrorCode::GraphicsApiUnavailable,
            stream.str());
    }

    void OpenGLContext::read_capabilities()
    {
        /*
         * Capability strings and integer limits are cached once during context
         * initialization so later systems can query them without touching OpenGL.
         */
        capabilities_.vendor = safe_gl_string(glGetString(GL_VENDOR));
        capabilities_.renderer = safe_gl_string(glGetString(GL_RENDERER));
        capabilities_.version = safe_gl_string(glGetString(GL_VERSION));
        capabilities_.shadingLanguageVersion = safe_gl_string(glGetString(GL_SHADING_LANGUAGE_VERSION));

        glGetIntegerv(GL_MAJOR_VERSION, &capabilities_.majorVersion);
        glGetIntegerv(GL_MINOR_VERSION, &capabilities_.minorVersion);

        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &capabilities_.maxTextureSize);
        glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &capabilities_.maxVertexAttributes);
        glGetIntegerv(GL_MAX_UNIFORM_BUFFER_BINDINGS, &capabilities_.maxUniformBufferBindings);

        capabilities_.debugOutputSupported =
            capabilities_.majorVersion > 4 ||
            (capabilities_.majorVersion == 4 && capabilities_.minorVersion >= 3);
    }

    void OpenGLContext::configure_debug_output(bool enabled)
    {
        if (!enabled || !capabilities_.debugOutputSupported)
        {
            return;
        }

        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(opengl_debug_callback, nullptr);
    }

}
