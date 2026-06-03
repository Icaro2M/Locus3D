#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/window/Window.h"

#include <glad/glad.h>

#include <iostream>
#include <string>

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

    const std::string vertexShaderSource = R"(
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;

out vec4 v_Color;

void main()
{
    v_Color = a_Color;
    gl_Position = vec4(a_Position, 1.0);
}
)";

    const std::string fragmentShaderSource = R"(
#version 450 core

in vec4 v_Color;

out vec4 FragColor;

void main()
{
    FragColor = v_Color;
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

    locus::graphics::MeshUploadData meshData;

    meshData.vertices = {
        {
            { -0.5f, -0.5f, 0.0f },
            {  0.0f,  0.0f, 1.0f },
            {  1.0f,  0.4f, 0.2f, 1.0f }
        },
        {
            {  0.5f, -0.5f, 0.0f },
            {  0.0f,  0.0f, 1.0f },
            {  0.2f,  0.8f, 1.0f, 1.0f }
        },
        {
            {  0.0f,  0.5f, 0.0f },
            {  0.0f,  0.0f, 1.0f },
            {  0.9f,  0.9f, 0.2f, 1.0f }
        }
    };

    meshData.topology = locus::graphics::PrimitiveTopology::Triangles;
    meshData.usage = locus::graphics::BufferUsage::Static;

    locus::graphics::MeshUploader meshUploader;

    auto meshResult = meshUploader.upload(meshData);

    if (!meshResult)
    {
        std::cerr << meshResult.error().message << '\n';
        return 1;
    }

    locus::graphics::GpuMesh gpuMesh = meshResult.move_value();

    while (!window.should_close())
    {
        window.poll_events();

        glViewport(
            0,
            0,
            window.framebuffer_width(),
            window.framebuffer_height());

        glClearColor(
            graphicsConfig.defaultClearColor.r,
            graphicsConfig.defaultClearColor.g,
            graphicsConfig.defaultClearColor.b,
            graphicsConfig.defaultClearColor.a);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.bind();
        gpuMesh.draw();
        shader.unbind();

        context.swap_buffers();
    }

    gpuMesh.destroy();
    shader.destroy();
    context.shutdown();
    window.destroy();

    return 0;
}