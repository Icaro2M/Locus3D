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
#include "kernel/geometry/primitives/BoxBuilder.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/operations/face/FlipFaceOp.h"
#include "kernel/modeling/operations/transform/TransformOp.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>

namespace {

    locus::graphics::MeshUploadData to_upload_data(
        const locus::kernel::geometry::RenderMesh& renderMesh,
        const glm::vec4& color)
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
            meshVertex.color[0] = color.r;
            meshVertex.color[1] = color.g;
            meshVertex.color[2] = color.b;
            meshVertex.color[3] = color.a;
            uploadData.vertices.push_back(meshVertex);
        }

        for (const locus::kernel::geometry::RenderTriangle& triangle : renderMesh.triangles) {
            uploadData.indices.push_back(triangle.a);
            uploadData.indices.push_back(triangle.b);
            uploadData.indices.push_back(triangle.c);
        }

        return uploadData;
    }

    void print_report(const char* label, const locus::kernel::geometry::LEM& mesh)
    {
        const locus::kernel::geometry::TopologyValidationReport report =
            locus::kernel::geometry::TopologyValidator::validate(mesh);

        std::cout << label << '\n';
        std::cout << "vertices: " << mesh.vertex_count() << '\n';
        std::cout << "edges: " << mesh.edge_count() << '\n';
        std::cout << "loops: " << mesh.loop_count() << '\n';
        std::cout << "faces: " << mesh.face_count() << '\n';
        std::cout << "topology valid: " << (report.valid() ? "yes" : "no") << '\n';
        std::cout << "errors: " << report.error_count() << '\n';
        std::cout << "warnings: " << report.warning_count() << "\n\n";
    }

    locus::kernel::geometry::RenderMesh build_render_mesh(locus::kernel::geometry::LEM& mesh)
    {
        locus::kernel::geometry::NormalBuilder::rebuild_face_normals(mesh);

        locus::kernel::geometry::RenderMesh renderMesh =
            locus::kernel::geometry::MeshTriangulator::triangulate(mesh);

        locus::kernel::geometry::NormalBuilder::rebuild_normals(
            renderMesh,
            locus::kernel::geometry::NormalBuildMode::Flat
        );

        return renderMesh;
    }

}

int main()
{
    using namespace locus::kernel::geometry;
    using namespace locus::kernel::modeling;

    BoxParameters boxParameters;
    boxParameters.center = { 0.0f, 0.5f, 0.0f };
    boxParameters.size = { 1.6f, 1.0f, 1.6f };

    LEM boxMesh;
    PrimitiveBuildResult buildResult = BoxBuilder::build_into(boxMesh, boxParameters);

    std::cout << "box build success: " << (buildResult.success ? "yes" : "no") << '\n';
    std::cout << "build diff changes: " << buildResult.diff.size() << "\n\n";

    print_report("box mesh", boxMesh);

    LEM transformedBoxMesh = boxMesh;

    glm::mat4 transform{ 1.0f };
    transform = glm::scale(transform, glm::vec3{ 1.0f, 1.6f, 1.0f });
    transform = glm::rotate(transform, 0.45f, glm::vec3{ 0.0f, 1.0f, 0.0f });

    TransformOp transformOp(transform);

    OperationContext transformContext;
    transformContext.mesh = &transformedBoxMesh;
    transformContext.validateAfterExecute = true;
    transformContext.rebuildNormals = true;
    transformContext.allowNonManifold = true;

    OperationResult transformResult = transformOp.execute(transformContext);

    std::cout << "transform success: " << (transformResult.is_success() ? "yes" : "no") << '\n';
    std::cout << "transform changed: " << (transformResult.changed() ? "yes" : "no") << '\n';
    std::cout << "transform diff changes: " << transformResult.diff().size() << "\n\n";

    FlipFaceOp flipFaceOp({ FaceHandle(0) });

    OperationContext flipContext;
    flipContext.mesh = &transformedBoxMesh;
    flipContext.validateAfterExecute = true;
    flipContext.rebuildNormals = true;
    flipContext.allowNonManifold = true;

    OperationResult flipResult = flipFaceOp.execute(flipContext);

    std::cout << "flip face 0 success: " << (flipResult.is_success() ? "yes" : "no") << '\n';
    std::cout << "flip face 0 changed: " << (flipResult.changed() ? "yes" : "no") << '\n';
    std::cout << "flip diff changes: " << flipResult.diff().size() << "\n\n";

    print_report("transformed box mesh", transformedBoxMesh);

    locus::graphics::GraphicsConfig graphicsConfig;
    graphicsConfig.requestedMajorVersion = 4;
    graphicsConfig.requestedMinorVersion = 5;
    graphicsConfig.enableDebugOutput = true;
    graphicsConfig.enableVSync = true;

    locus::graphics::WindowCreateInfo windowInfo;
    windowInfo.width = 1280;
    windowInfo.height = 720;
    windowInfo.title = "Locus3D - BoxBuilder Visual Test";
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
    vec4 baseColor = v_Color;

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

    RenderMesh boxRenderMesh = build_render_mesh(boxMesh);
    RenderMesh transformedBoxRenderMesh = build_render_mesh(transformedBoxMesh);

    locus::graphics::MeshUploadData boxUploadData =
        to_upload_data(boxRenderMesh, { 0.2f, 0.75f, 1.0f, 1.0f });

    locus::graphics::MeshUploadData transformedBoxUploadData =
        to_upload_data(transformedBoxRenderMesh, { 1.0f, 0.55f, 0.15f, 1.0f });

    locus::graphics::MeshUploader meshUploader;

    auto boxGpuMeshResult = meshUploader.upload(boxUploadData);

    if (!boxGpuMeshResult) {
        std::cerr << boxGpuMeshResult.error().message << '\n';
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    auto transformedBoxGpuMeshResult = meshUploader.upload(transformedBoxUploadData);

    if (!transformedBoxGpuMeshResult) {
        std::cerr << transformedBoxGpuMeshResult.error().message << '\n';
        boxGpuMeshResult.value().destroy();
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::GpuMesh boxGpuMesh = boxGpuMeshResult.move_value();
    locus::graphics::GpuMesh transformedBoxGpuMesh = transformedBoxGpuMeshResult.move_value();

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
        transformedBoxGpuMesh.destroy();
        boxGpuMesh.destroy();
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
        transformedBoxGpuMesh.destroy();
        boxGpuMesh.destroy();
        shader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::RenderObject boxObject;
    boxObject.id = 1;
    boxObject.name = "BoxBuilder Box";
    boxObject.mesh = &boxGpuMesh;
    boxObject.shader = &shader;
    boxObject.layer = locus::graphics::RenderLayer::Default;
    boxObject.transform.position = { -1.6f, 0.0f, 0.0f };

    locus::graphics::RenderObject transformedBoxObject;
    transformedBoxObject.id = 2;
    transformedBoxObject.name = "Transformed Box";
    transformedBoxObject.mesh = &transformedBoxGpuMesh;
    transformedBoxObject.shader = &shader;
    transformedBoxObject.layer = locus::graphics::RenderLayer::Default;
    transformedBoxObject.transform.position = { 1.6f, 0.0f, 0.0f };

    locus::graphics::Viewport viewport;
    viewport.set_clear_color(graphicsConfig.defaultClearColor);
    viewport.camera().projection().set_perspective(
        0.78539816339f,
        16.0f / 9.0f,
        0.01f,
        1000.0f
    );

    locus::graphics::OrbitCameraRig orbitRig;
    orbitRig.set_target({ 0.0f, 0.6f, 0.0f });
    orbitRig.set_distance(7.0f);
    orbitRig.set_angles(0.75f, 0.7f);
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
        pipeline.reserve(4);
        pipeline.submit(gridRenderer.render_object());
        pipeline.submit(boxObject);
        pipeline.submit(transformedBoxObject);
        pipeline.submit(axisRenderer.render_object());

        viewport.begin_frame();
        pipeline.render(renderer);

        context.swap_buffers();
    }

    axisRenderer.destroy();
    gridRenderer.destroy();
    transformedBoxGpuMesh.destroy();
    boxGpuMesh.destroy();
    shader.destroy();
    shaderManager.clear();
    context.shutdown();
    window.destroy();

    return 0;
}