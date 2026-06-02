#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/window/Window.h"

#include <glad/glad.h>

#include <iostream>

int main()
{
    locus::graphics::GraphicsConfig graphicsConfig;
    graphicsConfig.requestedMajorVersion = 4;
    graphicsConfig.requestedMinorVersion = 5;
    graphicsConfig.enableDebugOutput = true;
    graphicsConfig.enableVSync = true;

    locus::graphics::WindowCreateInfo windowInfo;
    windowInfo.width = 1280;
    windowInfo.height = 720;
    windowInfo.title = "Locus3D";
    windowInfo.openglMajorVersion = graphicsConfig.requestedMajorVersion;
    windowInfo.openglMinorVersion = graphicsConfig.requestedMinorVersion;
    windowInfo.openglDebugContext = graphicsConfig.enableDebugOutput;
    windowInfo.openglCoreProfile = graphicsConfig.coreProfile;
    windowInfo.openglForwardCompatible = graphicsConfig.forwardCompatible;

    locus::graphics::Window window;

    auto windowResult = window.create(windowInfo);

    if (!windowResult)
    {
        std::cerr << windowResult.error().message << '\n';
        return 1;
    }

    locus::graphics::OpenGLContext context;

    auto contextResult = context.initialize(window, graphicsConfig);

    if (!contextResult)
    {
        std::cerr << contextResult.error().message << '\n';
        return 1;
    }

    const auto& capabilities = context.capabilities();

    std::cout << "OpenGL Vendor: " << capabilities.vendor << '\n';
    std::cout << "OpenGL Renderer: " << capabilities.renderer << '\n';
    std::cout << "OpenGL Version: " << capabilities.version << '\n';
    std::cout << "GLSL Version: " << capabilities.shadingLanguageVersion << '\n';

    while (!window.should_close())
    {
        window.poll_events();

        glClearColor(
            graphicsConfig.defaultClearColor.r,
            graphicsConfig.defaultClearColor.g,
            graphicsConfig.defaultClearColor.b,
            graphicsConfig.defaultClearColor.a);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        context.swap_buffers();
    }

    context.shutdown();
    window.destroy();

    return 0;
}