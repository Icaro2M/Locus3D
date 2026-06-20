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
#include "kernel/geometry/primitives/ConeBuilder.h"
#include "kernel/geometry/primitives/CylinderBuilder.h"
#include "kernel/geometry/primitives/SphereBuilder.h"
#include "kernel/geometry/primitives/TorusBuilder.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyValidator.h"

#include <glm/glm.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace {

    locus::graphics::MeshUploadData to_upload_data(
        const locus::kernel::geometry::RenderMesh& renderMesh,
        const glm::vec4& color) {
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

    void print_report(
        const char* label,
        const locus::kernel::geometry::LEM& mesh) {
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

    locus::kernel::geometry::RenderMesh build_render_mesh(
        locus::kernel::geometry::LEM& mesh,
        locus::kernel::geometry::NormalBuildMode normalMode) {
        locus::kernel::geometry::NormalBuilder::rebuild_face_normals(mesh);

        locus::kernel::geometry::RenderMesh renderMesh =
            locus::kernel::geometry::MeshTriangulator::triangulate(mesh);

        locus::kernel::geometry::NormalBuilder::rebuild_normals(renderMesh, normalMode);

        return renderMesh;
    }

    bool add_visual_primitive(
        const std::string& name,
        locus::kernel::geometry::LEM& mesh,
        const glm::vec3& position,
        const glm::vec4& color,
        locus::kernel::geometry::NormalBuildMode normalMode,
        locus::graphics::MeshUploader& meshUploader,
        locus::graphics::Shader& shader,
        std::vector<locus::graphics::GpuMesh>& gpuMeshes,
        std::vector<locus::graphics::RenderObject>& renderObjects) {
        print_report(name.c_str(), mesh);

        locus::kernel::geometry::RenderMesh renderMesh = build_render_mesh(mesh, normalMode);

        std::cout << name << " render mesh\n";
        std::cout << "render vertices: " << renderMesh.vertex_count() << '\n';
        std::cout << "render triangles: " << renderMesh.triangle_count() << "\n\n";

        locus::graphics::MeshUploadData uploadData = to_upload_data(renderMesh, color);

        auto gpuMeshResult = meshUploader.upload(uploadData);

        if (!gpuMeshResult) {
            std::cerr << gpuMeshResult.error().message << '\n';
            return false;
        }

        gpuMeshes.push_back(gpuMeshResult.move_value());

        locus::graphics::RenderObject object;
        object.id = static_cast<locus::graphics::RenderObject::Id>(renderObjects.size() + 1);
        object.name = name;
        object.mesh = &gpuMeshes.back();
        object.shader = &shader;
        object.layer = locus::graphics::RenderLayer::Default;
        object.transform.position = position;

        renderObjects.push_back(object);

        return true;
    }

    void destroy_gpu_meshes(std::vector<locus::graphics::GpuMesh>& gpuMeshes) {
        for (locus::graphics::GpuMesh& mesh : gpuMeshes) {
            mesh.destroy();
        }

        gpuMeshes.clear();
    }

}

int main() {
    using namespace locus::kernel::geometry;

    BoxParameters boxParameters;
    boxParameters.center = { 0.0f, 0.5f, 0.0f };
    boxParameters.size = { 1.2f, 1.0f, 1.2f };

    CylinderParameters cylinderParameters;
    cylinderParameters.center = { 0.0f, 0.5f, 0.0f };
    cylinderParameters.radius = 0.55f;
    cylinderParameters.height = 1.2f;
    cylinderParameters.segments = 32;
    cylinderParameters.capTop = true;
    cylinderParameters.capBottom = true;

    SphereParameters sphereParameters;
    sphereParameters.center = { 0.0f, 0.65f, 0.0f };
    sphereParameters.radius = 0.65f;
    sphereParameters.longitudeSegments = 32;
    sphereParameters.latitudeSegments = 16;

    ConeParameters coneParameters;
    coneParameters.center = { 0.0f, 0.6f, 0.0f };
    coneParameters.radius = 0.6f;
    coneParameters.height = 1.2f;
    coneParameters.segments = 32;
    coneParameters.capBottom = true;

    TorusParameters torusParameters;
    torusParameters.center = { 0.0f, 0.7f, 0.0f };
    torusParameters.majorRadius = 0.55f;
    torusParameters.minorRadius = 0.18f;
    torusParameters.majorSegments = 48;
    torusParameters.minorSegments = 16;

    LEM boxMesh;
    LEM cylinderMesh;
    LEM sphereMesh;
    LEM coneMesh;
    LEM torusMesh;

    PrimitiveBuildResult boxResult = BoxBuilder::build_into(boxMesh, boxParameters);
    PrimitiveBuildResult cylinderResult = CylinderBuilder::build_into(cylinderMesh, cylinderParameters);
    PrimitiveBuildResult sphereResult = SphereBuilder::build_into(sphereMesh, sphereParameters);
    PrimitiveBuildResult coneResult = ConeBuilder::build_into(coneMesh, coneParameters);
    PrimitiveBuildResult torusResult = TorusBuilder::build_into(torusMesh, torusParameters);

    std::cout << "primitive build results\n";
    std::cout << "box: " << (boxResult.success ? "yes" : "no") << '\n';
    std::cout << "cylinder: " << (cylinderResult.success ? "yes" : "no") << '\n';
    std::cout << "sphere: " << (sphereResult.success ? "yes" : "no") << '\n';
    std::cout << "cone: " << (coneResult.success ? "yes" : "no") << '\n';
    std::cout << "torus: " << (torusResult.success ? "yes" : "no") << "\n\n";

    if (!boxResult.success || !cylinderResult.success || !sphereResult.success || !coneResult.success || !torusResult.success) {
        return 1;
    }

    locus::graphics::GraphicsConfig graphicsConfig;
    graphicsConfig.requestedMajorVersion = 4;
    graphicsConfig.requestedMinorVersion = 5;
    graphicsConfig.enableDebugOutput = true;
    graphicsConfig.enableVSync = true;

    locus::graphics::WindowCreateInfo windowInfo;
    windowInfo.width = 1280;
    windowInfo.height = 720;
    windowInfo.title = "Locus3D - Primitive Builders Visual Test";
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
    std::cout << "GLSL Version: " << capabilities.shadingLanguageVersion << "\n\n";

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

void main() {
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

void main() {
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

    locus::graphics::Shader primitiveShader;
    auto shaderResult = primitiveShader.create_from_source(vertexShaderSource, fragmentShaderSource);

    if (!shaderResult) {
        std::cerr << shaderResult.error().message << '\n';
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::MeshUploader meshUploader;

    std::vector<locus::graphics::GpuMesh> gpuMeshes;
    gpuMeshes.reserve(5);

    std::vector<locus::graphics::RenderObject> primitiveObjects;
    primitiveObjects.reserve(5);

    if (!add_visual_primitive(
        "Box",
        boxMesh,
        { -4.0f, 0.0f, 0.0f },
        { 0.2f, 0.75f, 1.0f, 1.0f },
        NormalBuildMode::Flat,
        meshUploader,
        primitiveShader,
        gpuMeshes,
        primitiveObjects)) {
        primitiveShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    if (!add_visual_primitive(
        "Cylinder",
        cylinderMesh,
        { -2.0f, 0.0f, 0.0f },
        { 0.35f, 0.95f, 0.45f, 1.0f },
        NormalBuildMode::Smooth,
        meshUploader,
        primitiveShader,
        gpuMeshes,
        primitiveObjects)) {
        destroy_gpu_meshes(gpuMeshes);
        primitiveShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    if (!add_visual_primitive(
        "Sphere",
        sphereMesh,
        { 0.0f, 0.0f, 0.0f },
        { 1.0f, 0.78f, 0.25f, 1.0f },
        NormalBuildMode::Smooth,
        meshUploader,
        primitiveShader,
        gpuMeshes,
        primitiveObjects)) {
        destroy_gpu_meshes(gpuMeshes);
        primitiveShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    if (!add_visual_primitive(
        "Cone",
        coneMesh,
        { 2.0f, 0.0f, 0.0f },
        { 1.0f, 0.45f, 0.25f, 1.0f },
        NormalBuildMode::Smooth,
        meshUploader,
        primitiveShader,
        gpuMeshes,
        primitiveObjects)) {
        destroy_gpu_meshes(gpuMeshes);
        primitiveShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    if (!add_visual_primitive(
        "Torus",
        torusMesh,
        { 4.0f, 0.0f, 0.0f },
        { 0.75f, 0.45f, 1.0f, 1.0f },
        NormalBuildMode::Smooth,
        meshUploader,
        primitiveShader,
        gpuMeshes,
        primitiveObjects)) {
        destroy_gpu_meshes(gpuMeshes);
        primitiveShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

    locus::graphics::GridRendererConfig gridConfig;
    gridConfig.halfExtent = 500.0f;
    gridConfig.minorSpacing = 1.0f;
    gridConfig.majorSpacing = 5.0f;
    gridConfig.fadeStart = 80.0f;
    gridConfig.fadeEnd = 280.0f;

    locus::graphics::GridRenderer gridRenderer;
    auto gridResult = gridRenderer.create(meshUploader, shaderManager, gridConfig);

    if (!gridResult) {
        std::cerr << gridResult.error().message << '\n';
        destroy_gpu_meshes(gpuMeshes);
        primitiveShader.destroy();
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
    auto axisResult = axisRenderer.create(meshUploader, shaderManager, axisConfig);

    if (!axisResult) {
        std::cerr << axisResult.error().message << '\n';
        gridRenderer.destroy();
        destroy_gpu_meshes(gpuMeshes);
        primitiveShader.destroy();
        shaderManager.clear();
        context.shutdown();
        window.destroy();
        return 1;
    }

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
    orbitRig.set_distance(9.0f);
    orbitRig.set_angles(0.65f, 0.65f);
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
        pipeline.reserve(primitiveObjects.size() + 2);

        pipeline.submit(gridRenderer.render_object());

        for (const locus::graphics::RenderObject& object : primitiveObjects) {
            pipeline.submit(object);
        }

        pipeline.submit(axisRenderer.render_object());

        viewport.begin_frame();
        pipeline.render(renderer);

        context.swap_buffers();
    }

    axisRenderer.destroy();
    gridRenderer.destroy();
    destroy_gpu_meshes(gpuMeshes);
    primitiveShader.destroy();
    shaderManager.clear();
    context.shutdown();
    window.destroy();

    return 0;
}