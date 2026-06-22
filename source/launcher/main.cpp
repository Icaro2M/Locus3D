#define NOMINMAX

#include "graphics/common/GraphicsConfig.h"
#include "graphics/context/OpenGLContext.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploadData.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"
#include "graphics/window/Window.h"

#include "kernel/common/Result.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/render/WireframeBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/geometry/topology/TopologyValidator.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/face/FlipFaceOp.h"
#include "kernel/modeling/operations/topology/MergeVerticesOp.h"
#include "kernel/modeling/operations/topology/SubdivideOp.h"

#include <glad/glad.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace {

    using namespace locus;
    using namespace locus::graphics;
    using namespace locus::kernel;
    using namespace locus::kernel::geometry;
    using namespace locus::kernel::modeling;

    const char* VisualVertexShader = R"(
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
    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    v_Normal = normalize(normalMatrix * a_Normal);
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

    const char* VisualFragmentShader = R"(
#version 450 core

in vec3 v_Normal;
in vec4 v_Color;

uniform vec4 u_BaseColor;
uniform int u_UseVertexColor;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 lightDirection = normalize(vec3(-0.35, -0.65, -0.75));
    float diffuse = max(dot(normal, -lightDirection), 0.0);
    float lighting = 0.30 + diffuse * 0.70;

    vec4 color = u_UseVertexColor != 0 ? v_Color : u_BaseColor;
    FragColor = vec4(color.rgb * lighting, color.a);
}
)";

    const char* WireVertexShader = R"(
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec4 a_Color;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec4 v_Color;

void main()
{
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";

    const char* WireFragmentShader = R"(
#version 450 core

in vec4 v_Color;

uniform vec4 u_BaseColor;
uniform int u_UseVertexColor;

out vec4 FragColor;

void main()
{
    FragColor = u_UseVertexColor != 0 ? v_Color : u_BaseColor;
}
)";

    struct MeshFit {
        glm::vec3 center{ 0.0f, 0.0f, 0.0f };
        float scale = 1.0f;
    };

    struct VisualMesh {
        std::string name;
        LEM mesh;
        RenderMesh solidRenderMesh;
        RenderMesh wireRenderMesh;
        MeshFit fit;
        MeshUploadData solidUploadData;
        MeshUploadData wireUploadData;
        GpuMesh solidGpuMesh;
        GpuMesh wireGpuMesh;
        glm::vec3 position{ 0.0f };
        glm::vec4 color{ 1.0f };
    };

    void print_graphics_error(const std::string& label, const GraphicsError& error)
    {
        std::cout << label << ": " << error.message << '\n';
    }

    const char* status_name(OperationStatus status)
    {
        switch (status) {
        case OperationStatus::Success:
            return "Success";
        case OperationStatus::Failed:
            return "Failed";
        case OperationStatus::NoChange:
            return "NoChange";
        case OperationStatus::Cancelled:
            return "Cancelled";
        }

        return "Unknown";
    }

    void print_operation_result(std::string_view label, const OperationResult& result)
    {
        std::cout << label
            << " | status: " << status_name(result.status())
            << " | changed: " << (result.changed() ? "true" : "false")
            << " | diff: " << result.diff().size();

        if (!result.message().empty()) {
            std::cout << " | message: " << result.message();
        }

        if (result.has_validation_report()) {
            const TopologyValidationReport& report = result.validation_report();
            std::cout
                << " | validation issues: " << report.issues.size()
                << " | errors: " << report.error_count()
                << " | warnings: " << report.warning_count();
        }

        std::cout << '\n';
    }

    void print_mesh_summary(std::string_view label, const LEM& mesh)
    {
        std::cout << label
            << " | vertices: " << TopologyTraversal::vertices(mesh).size()
            << " | edges: " << TopologyTraversal::edges(mesh).size()
            << " | loops: " << TopologyTraversal::loops(mesh).size()
            << " | faces: " << TopologyTraversal::faces(mesh).size()
            << '\n';
    }

    bool validate_mesh(std::string_view label, const LEM& mesh)
    {
        const TopologyValidationReport report = TopologyValidator::validate(mesh);

        std::cout << label
            << " | issues: " << report.issues.size()
            << " | errors: " << report.error_count()
            << " | warnings: " << report.warning_count()
            << '\n';

        return report.valid();
    }

    OperationContext make_context(LEM& mesh)
    {
        OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;
        return context;
    }

    MeshFit compute_mesh_fit(const RenderMesh& renderMesh)
    {
        if (renderMesh.vertices.empty()) {
            return {};
        }

        glm::vec3 minPoint{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()
        };

        glm::vec3 maxPoint{
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest()
        };

        for (const RenderVertex& vertex : renderMesh.vertices) {
            minPoint = glm::min(minPoint, vertex.position);
            maxPoint = glm::max(maxPoint, vertex.position);
        }

        const glm::vec3 size = maxPoint - minPoint;
        const float maxDimension = std::max({ size.x, size.y, size.z });

        MeshFit fit;
        fit.center = (minPoint + maxPoint) * 0.5f;
        fit.scale = maxDimension > 0.00001f ? 2.15f / maxDimension : 1.0f;

        return fit;
    }

    MeshUploadData build_triangle_upload_data(
        const RenderMesh& renderMesh,
        const MeshFit& fit,
        const glm::vec4& color)
    {
        MeshUploadData uploadData;
        uploadData.topology = PrimitiveTopology::Triangles;
        uploadData.usage = BufferUsage::Static;
        uploadData.vertices.reserve(renderMesh.vertices.size());
        uploadData.indices.reserve(renderMesh.triangles.size() * 3);

        for (const RenderVertex& renderVertex : renderMesh.vertices) {
            const glm::vec3 fittedPosition = (renderVertex.position - fit.center) * fit.scale;

            MeshVertex vertex;
            vertex.position[0] = fittedPosition.x;
            vertex.position[1] = fittedPosition.y;
            vertex.position[2] = fittedPosition.z;

            vertex.normal[0] = renderVertex.normal.x;
            vertex.normal[1] = renderVertex.normal.y;
            vertex.normal[2] = renderVertex.normal.z;

            vertex.color[0] = color.r;
            vertex.color[1] = color.g;
            vertex.color[2] = color.b;
            vertex.color[3] = color.a;

            uploadData.vertices.push_back(vertex);
        }

        for (const RenderTriangle& triangle : renderMesh.triangles) {
            uploadData.indices.push_back(static_cast<std::uint32_t>(triangle.a));
            uploadData.indices.push_back(static_cast<std::uint32_t>(triangle.b));
            uploadData.indices.push_back(static_cast<std::uint32_t>(triangle.c));
        }

        return uploadData;
    }

    MeshUploadData build_wire_upload_data(
        const RenderMesh& wireRenderMesh,
        const MeshFit& fit)
    {
        MeshUploadData uploadData;
        uploadData.topology = PrimitiveTopology::Lines;
        uploadData.usage = BufferUsage::Static;
        uploadData.vertices.reserve(wireRenderMesh.vertices.size());
        uploadData.indices.reserve(wireRenderMesh.lines.size() * 2);

        for (const RenderVertex& renderVertex : wireRenderMesh.vertices) {
            const glm::vec3 fittedPosition = (renderVertex.position - fit.center) * fit.scale;

            MeshVertex vertex;
            vertex.position[0] = fittedPosition.x;
            vertex.position[1] = fittedPosition.y;
            vertex.position[2] = fittedPosition.z;

            vertex.normal[0] = renderVertex.normal.x;
            vertex.normal[1] = renderVertex.normal.y;
            vertex.normal[2] = renderVertex.normal.z;

            vertex.color[0] = 0.02f;
            vertex.color[1] = 0.02f;
            vertex.color[2] = 0.025f;
            vertex.color[3] = 1.0f;

            uploadData.vertices.push_back(vertex);
        }

        for (const RenderLine& line : wireRenderMesh.lines) {
            uploadData.indices.push_back(static_cast<std::uint32_t>(line.a));
            uploadData.indices.push_back(static_cast<std::uint32_t>(line.b));
        }

        return uploadData;
    }

    bool create_shader(
        Shader& shader,
        const char* vertexSource,
        const char* fragmentSource,
        const std::string& label)
    {
        GraphicsResult result = shader.create_from_source(vertexSource, fragmentSource);

        if (!result) {
            print_graphics_error(label, result.error());
            return false;
        }

        std::cout << "[OK] " << label << '\n';
        return true;
    }

    bool create_gpu_mesh(
        GpuMesh& mesh,
        const MeshUploadData& uploadData,
        const std::string& label)
    {
        GraphicsResult result = mesh.create(uploadData);

        if (!result) {
            print_graphics_error(label, result.error());
            return false;
        }

        std::cout << "[OK] " << label << '\n';
        return true;
    }

    bool initialize_graphics(Window& window, OpenGLContext& context)
    {
        std::cout << "\n=== Graphics setup ===\n";

        WindowCreateInfo windowInfo;
        windowInfo.width = 1280;
        windowInfo.height = 720;
        windowInfo.title = "Locus3D - Modeling Operations Visual Test";
        windowInfo.resizable = true;
        windowInfo.visible = true;
        windowInfo.requestOpenGLContext = true;
        windowInfo.openglMajorVersion = 4;
        windowInfo.openglMinorVersion = 5;
        windowInfo.openglCoreProfile = true;
        windowInfo.openglForwardCompatible = true;
        windowInfo.openglDebugContext = true;

        GraphicsResult windowResult = window.create(windowInfo);

        if (!windowResult) {
            print_graphics_error("Window error", windowResult.error());
            return false;
        }

        GraphicsConfig config;
        config.api = GraphicsApi::OpenGL;
        config.enableDebugOutput = true;
        config.enableVSync = true;
        config.requestedMajorVersion = 4;
        config.requestedMinorVersion = 5;
        config.coreProfile = true;
        config.forwardCompatible = true;

        GraphicsResult contextResult = context.initialize(window, config);

        if (!contextResult) {
            print_graphics_error("OpenGL context error", contextResult.error());
            return false;
        }

        context.make_current();
        context.set_vsync(true);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDisable(GL_CULL_FACE);
        glLineWidth(2.0f);
        glViewport(0, 0, window.framebuffer_width(), window.framebuffer_height());

        std::cout << "[OK] janela e contexto OpenGL inicializados\n";
        std::cout << "Framebuffer: "
            << window.framebuffer_width()
            << "x"
            << window.framebuffer_height()
            << '\n';

        return true;
    }

    VisualMesh build_merge_visual_mesh()
    {
        VisualMesh visual;
        visual.name = "MergeVerticesOp";
        visual.position = { -3.0f, 0.0f, 0.0f };
        visual.color = { 0.80f, 0.88f, 1.0f, 1.0f };

        LEMEditor editor(visual.mesh);

        VertexHandle v0 = editor.add_vertex({ -1.0f, -1.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, -1.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ -1.0f, 1.0f, 0.0f });
        VertexHandle v4 = editor.add_vertex({ -1.08f, 1.08f, 0.0f });

        editor.add_face({ v0, v1, v2, v3 });
        editor.clear_diff();

        print_mesh_summary("Merge antes", visual.mesh);

        OperationContext context = make_context(visual.mesh);
        MergeVerticesOp op(v4, v3, { -1.0f, 1.0f, 0.0f });
        OperationResult result = op.execute(context);

        print_operation_result("MergeVerticesOp", result);
        print_mesh_summary("Merge depois", visual.mesh);
        validate_mesh("Validacao MergeVerticesOp", visual.mesh);

        return visual;
    }

    VisualMesh build_flip_visual_mesh()
    {
        VisualMesh visual;
        visual.name = "FlipFaceOp";
        visual.position = { 0.0f, 0.0f, 0.0f };
        visual.color = { 1.0f, 0.82f, 0.72f, 1.0f };

        LEMEditor editor(visual.mesh);

        VertexHandle v0 = editor.add_vertex({ -1.0f, -1.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, -1.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ -1.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });
        editor.clear_diff();

        print_mesh_summary("Flip antes", visual.mesh);

        OperationContext context = make_context(visual.mesh);
        FlipFaceOp op(face);
        OperationResult result = op.execute(context);

        print_operation_result("FlipFaceOp", result);
        print_mesh_summary("Flip depois", visual.mesh);
        validate_mesh("Validacao FlipFaceOp", visual.mesh);

        return visual;
    }

    VisualMesh build_subdivide_visual_mesh()
    {
        VisualMesh visual;
        visual.name = "SubdivideOp";
        visual.position = { 3.0f, 0.0f, 0.0f };
        visual.color = { 0.78f, 1.0f, 0.82f, 1.0f };

        LEMEditor editor(visual.mesh);

        VertexHandle v0 = editor.add_vertex({ -1.0f, -1.0f, 0.0f });
        VertexHandle v1 = editor.add_vertex({ 1.0f, -1.0f, 0.0f });
        VertexHandle v2 = editor.add_vertex({ 1.0f, 1.0f, 0.0f });
        VertexHandle v3 = editor.add_vertex({ -1.0f, 1.0f, 0.0f });

        FaceHandle face = editor.add_face({ v0, v1, v2, v3 });
        editor.clear_diff();

        print_mesh_summary("Subdivide antes", visual.mesh);

        OperationContext context = make_context(visual.mesh);
        SubdivideOp op = SubdivideOp::face(face);
        OperationResult result = op.execute(context);

        print_operation_result("SubdivideOp", result);
        print_mesh_summary("Subdivide depois", visual.mesh);
        validate_mesh("Validacao SubdivideOp", visual.mesh);

        return visual;
    }

    bool prepare_visual_mesh(VisualMesh& visual)
    {
        visual.solidRenderMesh = MeshTriangulator::triangulate(visual.mesh);
        NormalBuilder::rebuild_normals(visual.solidRenderMesh, NormalBuildMode::Flat);
        visual.wireRenderMesh = WireframeBuilder::build(visual.mesh);

        if (visual.solidRenderMesh.vertex_count() == 0 || visual.solidRenderMesh.triangle_count() == 0) {
            std::cout << "[FAIL] RenderMesh solido vazio para " << visual.name << '\n';
            return false;
        }

        if (visual.wireRenderMesh.line_count() == 0) {
            std::cout << "[FAIL] WireRenderMesh vazio para " << visual.name << '\n';
            return false;
        }

        visual.fit = compute_mesh_fit(visual.solidRenderMesh);
        visual.solidUploadData = build_triangle_upload_data(
            visual.solidRenderMesh,
            visual.fit,
            visual.color);
        visual.wireUploadData = build_wire_upload_data(
            visual.wireRenderMesh,
            visual.fit);

        std::cout << "[OK] RenderMesh preparado para " << visual.name
            << " | triangles: " << visual.solidRenderMesh.triangle_count()
            << " | wire lines: " << visual.wireRenderMesh.line_count()
            << '\n';

        return true;
    }

    bool upload_visual_mesh(VisualMesh& visual)
    {
        if (!create_gpu_mesh(
            visual.solidGpuMesh,
            visual.solidUploadData,
            visual.name + " solid GpuMesh")) {
            return false;
        }

        if (!create_gpu_mesh(
            visual.wireGpuMesh,
            visual.wireUploadData,
            visual.name + " wire GpuMesh")) {
            return false;
        }

        return true;
    }

    void add_visual_mesh_to_scene(
        RenderScene& scene,
        const VisualMesh& visual,
        const Shader& solidShader,
        const Shader& wireShader,
        std::uint64_t& nextId,
        const glm::quat& rotation)
    {
        RenderObject solidObject;
        solidObject.id = nextId++;
        solidObject.name = visual.name + " Solid";
        solidObject.mesh = &visual.solidGpuMesh;
        solidObject.shader = &solidShader;
        solidObject.transform.position = visual.position;
        solidObject.transform.rotation = rotation;
        solidObject.transform.scale = { 1.0f, 1.0f, 1.0f };
        solidObject.layer = RenderLayer::Default;

        RenderObject wireObject;
        wireObject.id = nextId++;
        wireObject.name = visual.name + " Wire";
        wireObject.mesh = &visual.wireGpuMesh;
        wireObject.shader = &wireShader;
        wireObject.transform = solidObject.transform;
        wireObject.layer = RenderLayer::Overlay;

        scene.add_object(solidObject);
        scene.add_object(wireObject);
    }

    void render_loop(
        Window& window,
        Renderer& renderer,
        const std::array<VisualMesh, 3>& visuals,
        const Shader& solidShader,
        const Shader& wireShader)
    {
        const auto start = std::chrono::steady_clock::now();

        while (!window.should_close()) {
            window.poll_events();

            const int framebufferWidth = window.framebuffer_width();
            const int framebufferHeight = window.framebuffer_height();

            if (framebufferWidth <= 0 || framebufferHeight <= 0) {
                continue;
            }

            const auto now = std::chrono::steady_clock::now();
            const float time = std::chrono::duration<float>(now - start).count();

            glViewport(0, 0, framebufferWidth, framebufferHeight);
            glClearColor(0.075f, 0.075f, 0.085f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            const float aspect =
                static_cast<float>(framebufferWidth)
                / static_cast<float>(framebufferHeight);

            const glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),
                aspect,
                0.1f,
                100.0f);

            const glm::mat4 view = glm::lookAt(
                glm::vec3{ 0.0f, 2.5f, 8.0f },
                glm::vec3{ 0.0f, 0.0f, 0.0f },
                glm::vec3{ 0.0f, 1.0f, 0.0f });

            const glm::quat rotation =
                glm::angleAxis(time * 0.35f, glm::vec3{ 0.0f, 1.0f, 0.0f })
                * glm::angleAxis(glm::radians(18.0f), glm::vec3{ 1.0f, 0.0f, 0.0f });

            RenderScene scene;
            scene.reserve(6);

            std::uint64_t nextId = 1;

            for (const VisualMesh& visual : visuals) {
                add_visual_mesh_to_scene(
                    scene,
                    visual,
                    solidShader,
                    wireShader,
                    nextId,
                    rotation);
            }

            renderer.set_view_matrix(view);
            renderer.set_projection_matrix(projection);
            renderer.render(scene);

            window.swap_buffers();
        }
    }

}

int main()
{
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "=== Locus3D Modeling Operations Visual Test ===\n";

    std::array<VisualMesh, 3> visuals{
        build_merge_visual_mesh(),
        build_flip_visual_mesh(),
        build_subdivide_visual_mesh()
    };

    for (VisualMesh& visual : visuals) {
        if (!prepare_visual_mesh(visual)) {
            std::cout << "\nResultado final: FAIL\n";
            return EXIT_FAILURE;
        }
    }

    Window window;
    OpenGLContext context;

    if (!initialize_graphics(window, context)) {
        std::cout << "\nResultado final: FAIL\n";
        return EXIT_FAILURE;
    }

    Shader solidShader;
    Shader wireShader;

    if (!create_shader(
        solidShader,
        VisualVertexShader,
        VisualFragmentShader,
        "shader solido criado")) {
        std::cout << "\nResultado final: FAIL\n";
        return EXIT_FAILURE;
    }

    if (!create_shader(
        wireShader,
        WireVertexShader,
        WireFragmentShader,
        "shader wire criado")) {
        std::cout << "\nResultado final: FAIL\n";
        return EXIT_FAILURE;
    }

    for (VisualMesh& visual : visuals) {
        if (!upload_visual_mesh(visual)) {
            std::cout << "\nResultado final: FAIL\n";
            return EXIT_FAILURE;
        }
    }

    Renderer renderer;

    std::cout << "\n=== Visual result ===\n";
    std::cout << "Esquerda: MergeVerticesOp\n";
    std::cout << "Centro: FlipFaceOp\n";
    std::cout << "Direita: SubdivideOp\n";
    std::cout << "Cada objeto aparece com wireframe topologico por cima.\n";
    std::cout << "Feche a janela para encerrar o teste.\n";

    render_loop(window, renderer, visuals, solidShader, wireShader);

    std::cout << "\nResultado final: PASS\n";
    return EXIT_SUCCESS;
}