#include "graphics/camera/OrbitCameraRig.h"
#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"
#include "graphics/viewport/Viewport.h"
#include "graphics/window/Window.h"

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
        window.destroy();
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

uniform mat4 u_MVP;

out vec4 v_Color;

void main()
{
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
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
        fragmentShaderSource
    );

    if (!shaderResult)
    {
        std::cerr << shaderResult.error().message << '\n';
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::MeshUploadData meshData;

    meshData.vertices = {
        {
            { -0.5f, -0.5f, 0.0f },
            { 0.0f, 0.0f, 1.0f },
            { 1.0f, 0.4f, 0.2f, 1.0f }
        },
        {
            { 0.5f, -0.5f, 0.0f },
            { 0.0f, 0.0f, 1.0f },
            { 0.2f, 0.8f, 1.0f, 1.0f }
        },
        {
            { 0.0f, 0.5f, 0.0f },
            { 0.0f, 0.0f, 1.0f },
            { 0.9f, 0.9f, 0.2f, 1.0f }
        }
    };

    meshData.topology = locus::graphics::PrimitiveTopology::Triangles;
    meshData.usage = locus::graphics::BufferUsage::Static;

    locus::graphics::MeshUploader meshUploader;
    auto meshResult = meshUploader.upload(meshData);

    if (!meshResult)
    {
        std::cerr << meshResult.error().message << '\n';
        shader.destroy();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::GpuMesh gpuMesh = meshResult.move_value();

    locus::graphics::RenderScene scene;

    locus::graphics::RenderObject triangleObject;
    triangleObject.id = 1;
    triangleObject.name = "Triangle";
    triangleObject.mesh = &gpuMesh;
    triangleObject.shader = &shader;

    scene.add_object(triangleObject);

    locus::graphics::Viewport viewport;
    viewport.set_clear_color(graphicsConfig.defaultClearColor);
    viewport.camera().projection().set_perspective(
        0.78539816339f,
        16.0f / 9.0f,
        0.01f,
        1000.0f
    );

    locus::graphics::OrbitCameraRig orbitRig;
    orbitRig.set_target({ 0.0f, 0.0f, 0.0f });
    orbitRig.set_distance(2.5f);
    orbitRig.set_angles(0.0f, 0.25f);
    orbitRig.apply(viewport.camera());

    locus::graphics::Renderer renderer;

    while (!window.should_close())
    {
        window.poll_events();

        viewport.sync_with_window(window);

        orbitRig.apply(viewport.camera());

        renderer.set_view_matrix(viewport.camera().view_matrix());
        renderer.set_projection_matrix(viewport.camera().projection_matrix());

        viewport.begin_frame();

        renderer.render(scene);

        context.swap_buffers();
    }

    gpuMesh.destroy();
    shader.destroy();
    context.shutdown();
    window.destroy();

    return 0;
}