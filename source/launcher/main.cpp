#include "graphics/camera/OrbitCameraRig.h"
#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/gpu/Shader.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/lighting/LightEnvironment.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/overlay/renderers/AxisRenderer.h"
#include "graphics/overlay/renderers/GridRenderer.h"
#include "graphics/renderer/RenderPipeline.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/viewport/Viewport.h"
#include "graphics/window/Window.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/transform/TransformOp.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <vector>

namespace {

    locus::graphics::MeshUploadData to_upload_data(const locus::kernel::geometry::RenderMesh& renderMesh)
    {
        locus::graphics::MeshUploadData uploadData;
        uploadData.topology = locus::graphics::PrimitiveTopology::Triangles;
        uploadData.usage = locus::graphics::BufferUsage::Static;
        uploadData.vertices.reserve(renderMesh.vertices.size());
        uploadData.indices.reserve(renderMesh.triangles.size() * 3);

        for (const locus::kernel::geometry::RenderVertex& vertex : renderMesh.vertices) {
            locus::graphics::MeshVertex meshVertex{};
            meshVertex.position[0] = vertex.position.x;
            meshVertex.position[1] = vertex.position.y;
            meshVertex.position[2] = vertex.position.z;
            meshVertex.normal[0] = vertex.normal.x;
            meshVertex.normal[1] = vertex.normal.y;
            meshVertex.normal[2] = vertex.normal.z;
            meshVertex.color[0] = 0.2f;
            meshVertex.color[1] = 0.75f;
            meshVertex.color[2] = 1.0f;
            meshVertex.color[3] = 1.0f;
            uploadData.vertices.push_back(meshVertex);
        }

        for (const locus::kernel::geometry::RenderTriangle& triangle : renderMesh.triangles) {
            uploadData.indices.push_back(triangle.a);
            uploadData.indices.push_back(triangle.b);
            uploadData.indices.push_back(triangle.c);
        }

        return uploadData;
    }

    locus::kernel::geometry::LEM build_test_mesh()
    {
        using namespace locus::kernel::geometry;
        using namespace locus::kernel::modeling;

        LEM mesh;
        LEMEditor editor(mesh);

        const VertexHandle v0 = editor.add_vertex({ -1.0f, 0.0f, -1.0f });
        const VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, -1.0f });
        const VertexHandle v2 = editor.add_vertex({ 1.0f, 0.0f, 1.0f });
        const VertexHandle v3 = editor.add_vertex({ -1.0f, 0.0f, 1.0f });

        editor.add_face({ v0, v1, v2, v3 });

        editor.set_selected(v2, true);
        editor.set_selected(v3, true);

        glm::mat4 transform{ 1.0f };
        transform = glm::translate(transform, glm::vec3{ 0.0f, 1.0f, 0.0f });

        TransformOp transformOp(transform);
        transformOp.set_target(TransformTarget::SelectedVertices);

        OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;

        OperationResult result = transformOp.execute(context);

        std::cout << "operation success: " << (result.is_success() ? "yes" : "no") << '\n';
        std::cout << "operation changed: " << (result.changed() ? "yes" : "no") << '\n';
        std::cout << "diff changes: " << result.diff().size() << '\n';

        const TopologyValidationReport report = TopologyValidator::validate(mesh);

        std::cout << "topology valid: " << (report.valid() ? "yes" : "no") << '\n';
        std::cout << "errors: " << report.error_count() << '\n';
        std::cout << "warnings: " << report.warning_count() << '\n';

        return mesh;
    }

}

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
    windowInfo.title = "Locus3D - Geometry Visual Test";
    windowInfo.openglMajorVersion = graphicsConfig.requestedMajorVersion;
    windowInfo.openglMinorVersion = graphicsConfig.requestedMinorVersion;
    windowInfo.openglDebugContext = graphicsConfig.enableDebugOutput;
    windowInfo.openglCoreProfile = graphicsConfig.coreProfile;
    windowInfo.openglForwardCompatible = graphicsConfig.forwardCompatible;

    locus::graphics::Window window;

    auto windowResult = window.create(windowInfo);

    if (!windowResult) {
        std::cerr << windowResult.error().message << '\n';
        return 1;
    }

    locus::graphics::OpenGLContext context;

    auto contextResult = context.initialize(window, graphicsConfig);

    if (!contextResult) {
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

    if (!gridShaderResult) {
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

    if (!axisShaderResult) {
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

    if (u_UseVertexColor == 1) {
        baseColor = v_Color;
    }

    if (u_ShadingMode == 0) {
        FragColor = baseColor;
        return;
    }

    if (u_ShadingMode == 2) {
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

    if (!shaderResult) {
        std::cerr << shaderResult.error().message << '\n';
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::kernel::geometry::LEM mesh = build_test_mesh();

    locus::kernel::geometry::NormalBuilder::rebuild_face_normals(mesh);

    locus::kernel::geometry::RenderMesh renderMesh =
        locus::kernel::geometry::MeshTriangulator::triangulate(mesh);

    locus::kernel::geometry::NormalBuilder::rebuild_normals(
        renderMesh,
        locus::kernel::geometry::NormalBuildMode::Flat
    );

    locus::graphics::MeshUploadData uploadData = to_upload_data(renderMesh);

    locus::graphics::MeshUploader meshUploader;

    auto gpuMeshResult = meshUploader.upload(uploadData);

    if (!gpuMeshResult) {
        std::cerr << gpuMeshResult.error().message << '\n';
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::GpuMesh gpuMesh = gpuMeshResult.move_value();

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

    if (!gridResult) {
        std::cerr << gridResult.error().message << '\n';
        gpuMesh.destroy();
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

    if (!axisResult) {
        std::cerr << axisResult.error().message << '\n';
        gridRenderer.destroy();
        gpuMesh.destroy();
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::RenderObject meshObject;
    meshObject.id = 1;
    meshObject.name = "Transformed LEM Quad";
    meshObject.mesh = &gpuMesh;
    meshObject.shader = &shader;
    meshObject.layer = locus::graphics::RenderLayer::Default;

    locus::graphics::Viewport viewport;
    viewport.set_clear_color(graphicsConfig.defaultClearColor);
    viewport.camera().projection().set_perspective(
        0.78539816339f,
        16.0f / 9.0f,
        0.01f,
        1000.0f
    );

    locus::graphics::OrbitCameraRig orbitRig;
    orbitRig.set_target({ 0.0f, 0.45f, 0.0f });
    orbitRig.set_distance(6.0f);
    orbitRig.set_angles(0.75f, 0.75f);
    orbitRig.apply(viewport.camera());

    locus::graphics::LightEnvironment lightEnvironment;
    lightEnvironment.reset_default_viewport_lighting();

    locus::graphics::Renderer renderer;
    renderer.set_light_environment(&lightEnvironment);

    locus::graphics::RenderPipeline pipeline;

    while (!window.should_close()) {
        window.poll_events();

        viewport.sync_with_window(window);
        orbitRig.apply(viewport.camera());

        gridRenderer.update(viewport.camera());

        renderer.set_view_matrix(viewport.camera().view_matrix());
        renderer.set_projection_matrix(viewport.camera().projection_matrix());

        pipeline.begin_frame();
        pipeline.reserve(3);
        pipeline.submit(gridRenderer.render_object());
        pipeline.submit(meshObject);
        pipeline.submit(axisRenderer.render_object());

        viewport.begin_frame();
        pipeline.render(renderer);

        context.swap_buffers();
    }

    axisRenderer.destroy();
    gridRenderer.destroy();
    gpuMesh.destroy();
    shader.destroy();
    shaderManager.clear();
    context.shutdown();
    window.destroy();

    return 0;
}