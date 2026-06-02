#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/window/Window.h"
#include "graphics/gpu/Buffer.h"
#include "graphics/gpu/VertexArray.h"
#include "graphics/gpu/Shader.h"

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

    const std::string vertexShaderSource = R"(
#version 450 core

layout (location = 0) in vec3 a_Position;

void main()
{
    gl_Position = vec4(a_Position, 1.0);
}
)";

    const std::string fragmentShaderSource = R"(
#version 450 core

out vec4 FragColor;

void main()
{
    FragColor = vec4(0.9, 0.5, 0.2, 1.0);
}
)";

    locus::graphics::Shader shader;

    auto shaderResult = shader.create_from_source(
        vertexShaderSource,
        fragmentShaderSource);

    if (!shaderResult)
    {
        std::cerr << shaderResult.error().message << '\n';
        return 1;
    }

    locus::graphics::Buffer vertexBuffer;
    auto vbResult = vertexBuffer.create(
        locus::graphics::BufferType::Vertex,
        locus::graphics::BufferUsage::Static);

    if (!vbResult)
    {
        std::cerr << vbResult.error().message << '\n';
        return 1;
    }

    locus::graphics::VertexArray vertexArray;
    auto vaoResult = vertexArray.create();

    if (!vaoResult)
    {
        std::cerr << vaoResult.error().message << '\n';
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