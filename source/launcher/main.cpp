#include "graphics/camera/CameraRayBuilder.h"
#include "graphics/camera/OrbitCameraRig.h"
#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/gpu/Shader.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/lighting/LightEnvironment.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/overlay/renderers/AxisRenderer.h"
#include "graphics/overlay/renderers/GridRenderer.h"
#include "graphics/picking/PickingBuffer.h"
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

    locus::graphics::ShaderManager shaderManager;
    shaderManager.set_shader_root("assets/shaders");

    auto gridShaderResult = shaderManager.load(
        "viewport/grid",
        "viewport/grid_vert.glsl",
        "viewport/grid_frag.glsl"
    );

    if (!gridShaderResult)
    {
        std::cerr << gridShaderResult.error().message << '\n';
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    auto axisShaderResult = shaderManager.load(
        "viewport/axis",
        "viewport/axis_vert.glsl",
        "viewport/axis_frag.glsl"
    );

    if (!axisShaderResult)
    {
        std::cerr << axisShaderResult.error().message << '\n';
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    const std::string vertexShaderSource = R"(
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec3 v_Normal;
out vec4 v_Color;

void main()
{
    v_Normal = mat3(transpose(inverse(u_Model))) * a_Normal;
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

    const std::string fragmentShaderSource = R"(
#version 450 core

in vec3 v_Normal;
in vec4 v_Color;

uniform vec4 u_BaseColor;
uniform int u_UseVertexColor;

uniform int u_ShadingMode;
uniform vec4 u_AmbientColor;
uniform float u_AmbientIntensity;
uniform vec3 u_LightDirection;
uniform vec4 u_LightColor;
uniform float u_LightIntensity;

out vec4 FragColor;

void main()
{
    vec4 baseColor = u_BaseColor;

    if (u_UseVertexColor == 1)
    {
        baseColor = v_Color;
    }

    if (u_ShadingMode == 0)
    {
        FragColor = baseColor;
        return;
    }

    if (u_ShadingMode == 2)
    {
        vec3 normalColor = normalize(v_Normal) * 0.5 + 0.5;
        FragColor = vec4(normalColor, baseColor.a);
        return;
    }

    vec3 normal = normalize(v_Normal);
    vec3 lightDirection = normalize(-u_LightDirection);

    float diffuse = max(dot(normal, lightDirection), 0.0);

    vec3 ambient = u_AmbientColor.rgb * u_AmbientIntensity;
    vec3 light = u_LightColor.rgb * diffuse * u_LightIntensity;
    vec3 finalColor = baseColor.rgb * (ambient + light);

    FragColor = vec4(finalColor, baseColor.a);
}
)";

    locus::graphics::Shader triangleShader;
    auto triangleShaderResult = triangleShader.create_from_source(
        vertexShaderSource,
        fragmentShaderSource
    );

    if (!triangleShaderResult)
    {
        std::cerr << triangleShaderResult.error().message << '\n';
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::MeshUploader meshUploader;

    locus::graphics::MeshUploadData triangleData;
    triangleData.vertices = {
        {
            { -0.5f, 0.05f, -0.5f },
            { 0.0f, 1.0f, 0.0f },
            { 1.0f, 0.4f, 0.2f, 1.0f }
        },
        {
            { 0.5f, 0.05f, -0.5f },
            { 0.0f, 1.0f, 0.0f },
            { 0.2f, 0.8f, 1.0f, 1.0f }
        },
        {
            { 0.0f, 0.05f, 0.5f },
            { 0.0f, 1.0f, 0.0f },
            { 0.9f, 0.9f, 0.2f, 1.0f }
        }
    };
    triangleData.topology = locus::graphics::PrimitiveTopology::Triangles;
    triangleData.usage = locus::graphics::BufferUsage::Static;

    auto triangleMeshResult = meshUploader.upload(triangleData);

    if (!triangleMeshResult)
    {
        std::cerr << triangleMeshResult.error().message << '\n';
        triangleShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::GpuMesh triangleMesh = triangleMeshResult.move_value();

    locus::graphics::GridRendererConfig gridConfig;
    gridConfig.halfExtent = 500.0f;
    gridConfig.minorSpacing = 1.0f;
    gridConfig.majorSpacing = 5.0f;
    gridConfig.fadeStart = 80.0f;
    gridConfig.fadeEnd = 280.0f;

    locus::graphics::GridRenderer gridRenderer;
    auto gridResult = gridRenderer.create(
        meshUploader,
        shaderManager,
        gridConfig
    );

    if (!gridResult)
    {
        std::cerr << gridResult.error().message << '\n';
        triangleMesh.destroy();
        triangleShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::AxisRendererConfig axisConfig;
    axisConfig.extent = 500.0f;
    axisConfig.verticalExtent = 40.0f;
    axisConfig.planeOffset = 0.004f;

    locus::graphics::AxisRenderer axisRenderer;
    auto axisResult = axisRenderer.create(
        meshUploader,
        shaderManager,
        axisConfig
    );

    if (!axisResult)
    {
        std::cerr << axisResult.error().message << '\n';
        gridRenderer.destroy();
        triangleMesh.destroy();
        triangleShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::RenderObject triangleObject;
    triangleObject.id = 1;
    triangleObject.name = "Triangle";
    triangleObject.mesh = &triangleMesh;
    triangleObject.shader = &triangleShader;
    triangleObject.layer = locus::graphics::RenderLayer::Default;

    locus::graphics::Viewport viewport;
    viewport.set_clear_color(graphicsConfig.defaultClearColor);
    viewport.sync_with_window(window);

    viewport.camera().projection().set_perspective(
        0.78539816339f,
        viewport.state().aspectRatio,
        0.01f,
        1000.0f
    );

    locus::graphics::OrbitCameraRig orbitRig;
    orbitRig.set_target({ 0.0f, 0.0f, 0.0f });
    orbitRig.set_distance(8.0f);
    orbitRig.set_angles(0.75f, 0.9f);
    orbitRig.apply(viewport.camera());

    locus::graphics::PickingBuffer pickingBuffer;

    auto pickingBufferResult = pickingBuffer.create(
        viewport.state().rect.width,
        viewport.state().rect.height
    );

    if (!pickingBufferResult)
    {
        std::cerr << pickingBufferResult.error().message << '\n';
        axisRenderer.destroy();
        gridRenderer.destroy();
        triangleMesh.destroy();
        triangleShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    std::cout << "PickingBuffer created: "
        << pickingBuffer.width()
        << "x"
        << pickingBuffer.height()
        << '\n';

    locus::graphics::LightEnvironment lightEnvironment;
    lightEnvironment.reset_default_viewport_lighting();

    locus::graphics::Renderer renderer;
    renderer.set_light_environment(&lightEnvironment);

    bool printedCenterRay = false;
    bool printedPickingRead = false;
    bool running = true;

    while (running && !window.should_close())
    {
        window.poll_events();

        viewport.sync_with_window(window);

        auto resizePickingResult = pickingBuffer.resize(
            viewport.state().rect.width,
            viewport.state().rect.height
        );

        if (!resizePickingResult)
        {
            std::cerr << resizePickingResult.error().message << '\n';
            running = false;
            continue;
        }

        orbitRig.apply(viewport.camera());
        gridRenderer.update(viewport.camera());

        renderer.set_view_matrix(viewport.camera().view_matrix());
        renderer.set_projection_matrix(viewport.camera().projection_matrix());

        if (!printedCenterRay)
        {
            const locus::graphics::ViewportRect rect = viewport.state().rect;

            const float centerX =
                static_cast<float>(rect.x) + static_cast<float>(rect.width) * 0.5f;

            const float centerY =
                static_cast<float>(rect.y) + static_cast<float>(rect.height) * 0.5f;

            const locus::graphics::CameraRay centerRay =
                locus::graphics::CameraRayBuilder::from_viewport_pixel(
                    viewport.camera(),
                    rect,
                    centerX,
                    centerY
                );

            std::cout << "Center ray origin: "
                << centerRay.origin.x << ", "
                << centerRay.origin.y << ", "
                << centerRay.origin.z << '\n';

            std::cout << "Center ray direction: "
                << centerRay.direction.x << ", "
                << centerRay.direction.y << ", "
                << centerRay.direction.z << '\n';

            printedCenterRay = true;
        }

        pickingBuffer.bind();
        pickingBuffer.clear();

        const locus::graphics::PickingId centerPick =
            pickingBuffer.read_id(
                pickingBuffer.width() / 2,
                pickingBuffer.height() / 2
            );

        locus::graphics::PickingBuffer::bind_default();

        if (!printedPickingRead)
        {
            std::cout << "Picking center id: " << centerPick.value << '\n';

            if (!centerPick.is_valid())
            {
                std::cout << "Picking center is invalid, as expected before PickingRenderer.\n";
            }

            printedPickingRead = true;
        }

        locus::graphics::RenderScene scene;
        scene.reserve(3);
        scene.add_object(gridRenderer.render_object());
        scene.add_object(triangleObject);
        scene.add_object(axisRenderer.render_object());

        viewport.begin_frame();

        renderer.render(scene);

        context.swap_buffers();
    }

    pickingBuffer.destroy();

    axisRenderer.destroy();
    gridRenderer.destroy();

    triangleMesh.destroy();
    triangleShader.destroy();

    shaderManager.clear();

    context.shutdown();
    window.destroy();

    return 0;
}