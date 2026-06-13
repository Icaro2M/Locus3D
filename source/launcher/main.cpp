#include "graphics/camera/OrbitCameraRig.h"
#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/debug/DebugDraw.h"
#include "graphics/gpu/Shader.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/lighting/LightEnvironment.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/overlay/renderers/AxisRenderer.h"
#include "graphics/overlay/renderers/BoundingBoxRenderer.h"
#include "graphics/overlay/renderers/GridRenderer.h"
#include "graphics/overlay/renderers/MeasurementRenderer.h"
#include "graphics/overlay/renderers/NormalRenderer.h"
#include "graphics/renderer/RenderPipeline.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
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

    auto boundingBoxShaderResult = shaderManager.load(
        "viewport/bounding_box",
        "viewport/bounding_box_vert.glsl",
        "viewport/bounding_box_frag.glsl"
    );

    if (!boundingBoxShaderResult)
    {
        std::cerr << boundingBoxShaderResult.error().message << '\n';
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    auto debugShaderResult = shaderManager.load(
        "debug/draw",
        "debug/debug_vert.glsl",
        "debug/debug_frag.glsl"
    );

    if (!debugShaderResult)
    {
        std::cerr << debugShaderResult.error().message << '\n';
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

    locus::graphics::Shader shader;
    auto shaderResult = shader.create_from_source(
        vertexShaderSource,
        fragmentShaderSource
    );

    if (!shaderResult)
    {
        std::cerr << shaderResult.error().message << '\n';
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
        shader.destroy();
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
        shader.destroy();
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
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::BoundingBoxRenderer boundingBoxRenderer;
    auto boundingBoxResult = boundingBoxRenderer.create(shaderManager);

    if (!boundingBoxResult)
    {
        std::cerr << boundingBoxResult.error().message << '\n';
        axisRenderer.destroy();
        gridRenderer.destroy();
        triangleMesh.destroy();
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::DebugDraw debugDraw;
    auto debugDrawResult = debugDraw.create(shaderManager);

    if (!debugDrawResult)
    {
        std::cerr << debugDrawResult.error().message << '\n';
        boundingBoxRenderer.destroy();
        axisRenderer.destroy();
        gridRenderer.destroy();
        triangleMesh.destroy();
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::NormalRenderer normalRenderer;
    auto normalRendererResult = normalRenderer.create(shaderManager);

    if (!normalRendererResult)
    {
        std::cerr << normalRendererResult.error().message << '\n';
        debugDraw.destroy();
        boundingBoxRenderer.destroy();
        axisRenderer.destroy();
        gridRenderer.destroy();
        triangleMesh.destroy();
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::MeasurementRenderer measurementRenderer;
    auto measurementRendererResult = measurementRenderer.create(shaderManager);

    if (!measurementRendererResult)
    {
        std::cerr << measurementRendererResult.error().message << '\n';
        normalRenderer.destroy();
        debugDraw.destroy();
        boundingBoxRenderer.destroy();
        axisRenderer.destroy();
        gridRenderer.destroy();
        triangleMesh.destroy();
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::RenderObject triangleObject;
    triangleObject.id = 1;
    triangleObject.name = "Triangle";
    triangleObject.mesh = &triangleMesh;
    triangleObject.shader = &shader;
    triangleObject.layer = locus::graphics::RenderLayer::Default;

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
    orbitRig.set_distance(8.0f);
    orbitRig.set_angles(0.75f, 0.9f);
    orbitRig.apply(viewport.camera());

    locus::graphics::LightEnvironment lightEnvironment;
    lightEnvironment.reset_default_viewport_lighting();

    locus::graphics::Renderer renderer;
    renderer.set_light_environment(&lightEnvironment);

    locus::graphics::RenderPipeline pipeline;

    while (!window.should_close())
    {
        window.poll_events();

        viewport.sync_with_window(window);

        orbitRig.apply(viewport.camera());
        gridRenderer.update(viewport.camera());

        renderer.set_view_matrix(viewport.camera().view_matrix());
        renderer.set_projection_matrix(viewport.camera().projection_matrix());

        boundingBoxRenderer.clear();
        boundingBoxRenderer.add_box(
            { -0.65f, 0.0f, -0.65f },
            { 0.65f, 0.25f, 0.65f },
            { 0.2f, 0.85f, 1.0f, 1.0f }
        );
        boundingBoxRenderer.add_box(
            { -1.25f, 0.0f, -1.25f },
            { 1.25f, 1.5f, 1.25f },
            { 1.0f, 0.85f, 0.15f, 1.0f }
        );

        auto boundingBoxUploadResult = boundingBoxRenderer.upload(meshUploader);
        if (!boundingBoxUploadResult)
        {
            std::cerr << boundingBoxUploadResult.error().message << '\n';
            break;
        }

        debugDraw.clear();
        debugDraw.add_line(
            { -2.0f, 1.0f, 0.0f },
            { 2.0f, 1.0f, 0.0f },
            { 1.0f, 0.85f, 0.15f, 1.0f }
        );
        debugDraw.add_ray(
            { 0.0f, 0.25f, 0.0f },
            { 0.0f, 1.0f, 0.0f },
            2.5f,
            { 0.2f, 1.0f, 0.35f, 1.0f }
        );

        auto debugUploadResult = debugDraw.upload(meshUploader);
        if (!debugUploadResult)
        {
            std::cerr << debugUploadResult.error().message << '\n';
            break;
        }

        normalRenderer.clear();
        normalRenderer.add_normal(
            { -0.5f, 0.05f, -0.5f },
            { 0.0f, 1.0f, 0.0f },
            0.8f,
            { 0.35f, 0.75f, 1.0f, 1.0f }
        );
        normalRenderer.add_normal(
            { 0.5f, 0.05f, -0.5f },
            { 0.0f, 1.0f, 0.0f },
            0.8f,
            { 0.35f, 0.75f, 1.0f, 1.0f }
        );
        normalRenderer.add_normal(
            { 0.0f, 0.05f, 0.5f },
            { 0.0f, 1.0f, 0.0f },
            0.8f,
            { 0.35f, 0.75f, 1.0f, 1.0f }
        );
        normalRenderer.add_normal(
            { 0.0f, 0.05f, 0.0f },
            { 0.0f, 1.0f, 0.0f },
            1.2f,
            { 1.0f, 0.35f, 0.85f, 1.0f }
        );

        auto normalUploadResult = normalRenderer.upload(meshUploader);
        if (!normalUploadResult)
        {
            std::cerr << normalUploadResult.error().message << '\n';
            break;
        }

        measurementRenderer.clear();
        measurementRenderer.add_measurement(
            { -0.5f, 0.08f, -0.5f },
            { 0.5f, 0.08f, -0.5f },
            { 1.0f, 0.85f, 0.15f, 1.0f }
        );
        measurementRenderer.add_measurement(
            { 0.5f, 0.08f, -0.5f },
            { 0.0f, 0.08f, 0.5f },
            { 1.0f, 0.85f, 0.15f, 1.0f }
        );
        measurementRenderer.add_measurement(
            { -1.25f, 1.55f, -1.25f },
            { 1.25f, 1.55f, -1.25f },
            { 0.2f, 1.0f, 0.35f, 1.0f }
        );

        auto measurementUploadResult = measurementRenderer.upload(meshUploader);
        if (!measurementUploadResult)
        {
            std::cerr << measurementUploadResult.error().message << '\n';
            break;
        }

        pipeline.begin_frame();
        pipeline.reserve(7);

        pipeline.submit(gridRenderer.render_object());
        pipeline.submit(triangleObject);
        pipeline.submit(axisRenderer.render_object());
        pipeline.submit(boundingBoxRenderer.render_object());
        pipeline.submit(debugDraw.render_object());
        pipeline.submit(normalRenderer.render_object());
        pipeline.submit(measurementRenderer.render_object());

        viewport.begin_frame();
        pipeline.render(renderer);

        context.swap_buffers();
    }

    measurementRenderer.destroy();
    normalRenderer.destroy();
    debugDraw.destroy();
    boundingBoxRenderer.destroy();
    axisRenderer.destroy();
    gridRenderer.destroy();

    triangleMesh.destroy();
    shader.destroy();

    shaderManager.clear();

    context.shutdown();
    window.destroy();

    return 0;
}