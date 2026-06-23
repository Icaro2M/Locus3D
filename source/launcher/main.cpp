#include "graphics/context/OpenGLContext.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploadData.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"
#include "graphics/viewport/Viewport.h"
#include "graphics/window/Window.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/render/WireframeBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/modeling/core/OperationContext.h"
#include "kernel/modeling/core/OperationResult.h"
#include "kernel/modeling/operations/face/ExtrudeFaceOp.h"
#include "kernel/modeling/operations/face/InsetFaceOp.h"
#include "kernel/modeling/operations/transform/RandomizeOp.h"
#include "kernel/modeling/operations/transform/ShrinkFattenOp.h"

#include <GLFW/glfw3.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

    using namespace locus;
    using namespace locus::graphics;
    using namespace locus::kernel::geometry;
    using namespace locus::kernel::modeling;

    struct VisualMesh {
        GpuMesh solid;
        GpuMesh wire;
    };

    struct DemoMesh {
        LEM mesh;
        std::string name;
        glm::vec3 position{ 0.0f };
    };

    const char* solid_vertex_shader()
    {
        return R"(
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color;

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
    }

    const char* solid_fragment_shader()
    {
        return R"(
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
    vec4 baseColor = (u_UseVertexColor != 0) ? v_Color : u_BaseColor;

    vec3 normal = normalize(v_Normal);
    vec3 lightDir = normalize(-u_LightDirection);

    float diffuse = max(dot(normal, lightDir), 0.0);
    vec3 ambient = u_AmbientColor.rgb * u_AmbientIntensity;
    vec3 light = u_LightColor.rgb * u_LightIntensity * diffuse;

    vec3 color = baseColor.rgb * max(ambient + light, vec3(0.25));
    FragColor = vec4(color, baseColor.a);
}
)";
    }

    const char* wire_vertex_shader()
    {
        return R"(
#version 450 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec4 a_Color;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec4 v_Color;

void main()
{
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";
    }

    const char* wire_fragment_shader()
    {
        return R"(
#version 450 core

in vec4 v_Color;

uniform vec4 u_BaseColor;
uniform int u_UseVertexColor;

out vec4 FragColor;

void main()
{
    FragColor = (u_UseVertexColor != 0) ? v_Color : u_BaseColor;
}
)";
    }

    void print_graphics_error(const std::string& label, const GraphicsError& error)
    {
        std::cout << "[Graphics Error] " << label << ": " << error.message << '\n';
    }

    void print_operation_warning(const std::string& label, const OperationResult& result)
    {
        if (result.is_success()) {
            return;
        }

        std::cout << "[WARN] " << label << " failed or produced no change";

        if (!result.message().empty()) {
            std::cout << ": " << result.message();
        }

        std::cout << '\n';
    }

    OperationContext make_operation_context(LEM& mesh)
    {
        OperationContext context;
        context.mesh = &mesh;
        context.validateAfterExecute = true;
        context.rebuildNormals = true;
        context.allowNonManifold = true;
        return context;
    }

    FaceHandle create_quad(LEMEditor& editor)
    {
        const VertexHandle v0 = editor.add_vertex({ -1.0f, 0.0f, -1.0f });
        const VertexHandle v1 = editor.add_vertex({ 1.0f, 0.0f, -1.0f });
        const VertexHandle v2 = editor.add_vertex({ 1.0f, 0.0f,  1.0f });
        const VertexHandle v3 = editor.add_vertex({ -1.0f, 0.0f,  1.0f });

        const FaceHandle face = editor.add_face({ v0, v1, v2, v3 });
        editor.rebuild_face_normals();

        return face;
    }

    FaceHandle first_face(const LEM& mesh)
    {
        const std::vector<FaceHandle> faces = TopologyTraversal::faces(mesh);

        if (faces.empty()) {
            return {};
        }

        return faces.front();
    }

    FaceHandle find_top_face(const LEM& mesh)
    {
        FaceHandle result;

        for (FaceHandle face : TopologyTraversal::faces(mesh)) {
            const std::vector<VertexHandle> vertices =
                TopologyTraversal::face_vertices(mesh, face);

            if (vertices.empty()) {
                continue;
            }

            bool allTop = true;

            for (VertexHandle vertex : vertices) {
                if (!mesh.is_valid(vertex)) {
                    allTop = false;
                    break;
                }

                if (mesh.vertex(vertex).position.y < 0.99f) {
                    allTop = false;
                    break;
                }
            }

            if (allTop) {
                result = face;
                break;
            }
        }

        return result;
    }

    DemoMesh make_original_mesh()
    {
        DemoMesh demo;
        demo.name = "Original";
        demo.position = { -4.5f, 0.0f, 0.0f };

        LEMEditor editor(demo.mesh);
        create_quad(editor);

        return demo;
    }

    DemoMesh make_shrink_fatten_mesh()
    {
        DemoMesh demo;
        demo.name = "Shrink/Fatten";
        demo.position = { -1.5f, 0.0f, 0.0f };

        LEMEditor editor(demo.mesh);
        create_quad(editor);

        OperationContext context = make_operation_context(demo.mesh);

        const FaceHandle face = first_face(demo.mesh);

        if (demo.mesh.is_valid(face)) {
            ExtrudeFaceOp extrude(face, { 0.0f, 1.0f, 0.0f });
            extrude.set_keep_source_face(false);

            const OperationResult extrudeResult = extrude.execute(context);
            print_operation_warning("Extrude in Shrink/Fatten mesh", extrudeResult);
        }

        ShrinkFattenOp shrinkFatten(0.25f);
        const OperationResult shrinkFattenResult = shrinkFatten.execute(context);
        print_operation_warning("Shrink/Fatten", shrinkFattenResult);

        return demo;
    }

    DemoMesh make_randomized_mesh()
    {
        DemoMesh demo;
        demo.name = "Randomize";
        demo.position = { 1.5f, 0.0f, 0.0f };

        LEMEditor editor(demo.mesh);
        create_quad(editor);

        OperationContext context = make_operation_context(demo.mesh);

        const FaceHandle face = first_face(demo.mesh);

        if (demo.mesh.is_valid(face)) {
            ExtrudeFaceOp extrude(face, { 0.0f, 1.0f, 0.0f });
            extrude.set_keep_source_face(false);

            const OperationResult extrudeResult = extrude.execute(context);
            print_operation_warning("Extrude in Randomize mesh", extrudeResult);
        }

        RandomizeOp randomize(0.20f);
        randomize.set_seed(2026u);

        const OperationResult randomizeResult = randomize.execute(context);
        print_operation_warning("Randomize", randomizeResult);

        return demo;
    }

    DemoMesh make_extrude_inset_mesh()
    {
        DemoMesh demo;
        demo.name = "Extrude + Inset";
        demo.position = { 4.5f, 0.0f, 0.0f };

        LEMEditor editor(demo.mesh);
        const FaceHandle baseFace = create_quad(editor);

        OperationContext context = make_operation_context(demo.mesh);

        if (demo.mesh.is_valid(baseFace)) {
            ExtrudeFaceOp extrude(baseFace, { 0.0f, 1.0f, 0.0f });
            extrude.set_keep_source_face(false);

            const OperationResult extrudeResult = extrude.execute(context);
            print_operation_warning("Extrude in Extrude + Inset mesh", extrudeResult);
        }

        const FaceHandle topFace = find_top_face(demo.mesh);

        if (demo.mesh.is_valid(topFace)) {
            InsetFaceOp inset(topFace, 0.35f);

            const OperationResult insetResult = inset.execute(context);
            print_operation_warning("Inset", insetResult);
        }
        else {
            std::cout << "[WARN] Top face not found for inset\n";
        }

        return demo;
    }

    MeshUploadData make_solid_upload_data(
        const RenderMesh& renderMesh,
        const ColorRGBA& color)
    {
        MeshUploadData uploadData;
        uploadData.topology = PrimitiveTopology::Triangles;
        uploadData.usage = BufferUsage::Static;

        uploadData.vertices.reserve(renderMesh.vertices.size());
        uploadData.indices.reserve(renderMesh.triangles.size() * 3);

        for (const RenderVertex& vertex : renderMesh.vertices) {
            MeshVertex gpuVertex;

            gpuVertex.position[0] = vertex.position.x;
            gpuVertex.position[1] = vertex.position.y;
            gpuVertex.position[2] = vertex.position.z;

            gpuVertex.normal[0] = vertex.normal.x;
            gpuVertex.normal[1] = vertex.normal.y;
            gpuVertex.normal[2] = vertex.normal.z;

            gpuVertex.color[0] = color.r;
            gpuVertex.color[1] = color.g;
            gpuVertex.color[2] = color.b;
            gpuVertex.color[3] = color.a;

            uploadData.vertices.push_back(gpuVertex);
        }

        for (const RenderTriangle& triangle : renderMesh.triangles) {
            uploadData.indices.push_back(triangle.a);
            uploadData.indices.push_back(triangle.b);
            uploadData.indices.push_back(triangle.c);
        }

        return uploadData;
    }

    MeshUploadData make_wire_upload_data(
        const RenderMesh& renderMesh,
        const ColorRGBA& color)
    {
        MeshUploadData uploadData;
        uploadData.topology = PrimitiveTopology::Lines;
        uploadData.usage = BufferUsage::Static;

        uploadData.vertices.reserve(renderMesh.vertices.size());
        uploadData.indices.reserve(renderMesh.lines.size() * 2);

        for (const RenderVertex& vertex : renderMesh.vertices) {
            MeshVertex gpuVertex;

            gpuVertex.position[0] = vertex.position.x;
            gpuVertex.position[1] = vertex.position.y;
            gpuVertex.position[2] = vertex.position.z;

            gpuVertex.normal[0] = 0.0f;
            gpuVertex.normal[1] = 1.0f;
            gpuVertex.normal[2] = 0.0f;

            gpuVertex.color[0] = color.r;
            gpuVertex.color[1] = color.g;
            gpuVertex.color[2] = color.b;
            gpuVertex.color[3] = color.a;

            uploadData.vertices.push_back(gpuVertex);
        }

        for (const RenderLine& line : renderMesh.lines) {
            uploadData.indices.push_back(line.a);
            uploadData.indices.push_back(line.b);
        }

        return uploadData;
    }

    bool upload_visual_mesh(const LEM& mesh, VisualMesh& output)
    {
        const RenderMesh solidRenderMesh = MeshTriangulator::triangulate(mesh);
        const RenderMesh wireRenderMesh = WireframeBuilder::build(mesh);

        const MeshUploadData solidUploadData =
            make_solid_upload_data(solidRenderMesh, { 0.65f, 0.78f, 1.0f, 1.0f });

        const MeshUploadData wireUploadData =
            make_wire_upload_data(wireRenderMesh, { 0.02f, 0.02f, 0.025f, 1.0f });

        auto solidResult = output.solid.create(solidUploadData);
        if (!solidResult) {
            print_graphics_error("solid mesh upload", solidResult.error());
            return false;
        }

        auto wireResult = output.wire.create(wireUploadData);
        if (!wireResult) {
            print_graphics_error("wire mesh upload", wireResult.error());
            return false;
        }

        return true;
    }

    RenderObject make_render_object(
        std::uint64_t id,
        const std::string& name,
        const GpuMesh& mesh,
        const Shader& shader,
        const glm::vec3& position,
        bool wireframe)
    {
        RenderObject object;
        object.id = id;
        object.name = name;
        object.mesh = &mesh;
        object.shader = &shader;
        object.transform.position = position;
        object.wireframe = wireframe;
        object.layer = RenderLayer::Default;
        return object;
    }

    bool initialize_graphics(
        Window& window,
        OpenGLContext& context,
        Viewport& viewport)
    {
        WindowCreateInfo windowInfo;
        windowInfo.width = 1280;
        windowInfo.height = 720;
        windowInfo.title = "Locus3D - Visual Modeling Operations Test";
        windowInfo.resizable = true;
        windowInfo.requestOpenGLContext = true;
        windowInfo.openglMajorVersion = 4;
        windowInfo.openglMinorVersion = 5;
        windowInfo.openglCoreProfile = true;
        windowInfo.openglForwardCompatible = true;
        windowInfo.openglDebugContext = true;

        auto windowResult = window.create(windowInfo);
        if (!windowResult) {
            print_graphics_error("window.create", windowResult.error());
            return false;
        }

        GraphicsConfig config;
        config.enableDebugOutput = true;
        config.enableVSync = true;
        config.requestedMajorVersion = 4;
        config.requestedMinorVersion = 5;
        config.coreProfile = true;
        config.forwardCompatible = true;

        auto contextResult = context.initialize(window, config);
        if (!contextResult) {
            print_graphics_error("context.initialize", contextResult.error());
            return false;
        }

        context.make_current();
        context.set_vsync(true);

        viewport.sync_with_window(window);
        viewport.set_clear_color({ 0.08f, 0.08f, 0.09f, 1.0f });
        viewport.set_depth_test_enabled(true);
        viewport.camera().look_at(
            { 0.0f, 4.0f, 9.0f },
            { 0.0f, 0.45f, 0.0f },
            { 0.0f, 1.0f, 0.0f });

        return true;
    }

}

int main()
{
    std::cout << "=== Locus3D Visual Modeling Operations Test ===\n";

    Window window;
    OpenGLContext context;
    Viewport viewport;

    if (!initialize_graphics(window, context, viewport)) {
        return EXIT_FAILURE;
    }

    Shader solidShader;
    Shader wireShader;

    auto solidShaderResult = solidShader.create_from_source(
        solid_vertex_shader(),
        solid_fragment_shader());

    if (!solidShaderResult) {
        print_graphics_error("solid shader", solidShaderResult.error());
        return EXIT_FAILURE;
    }

    auto wireShaderResult = wireShader.create_from_source(
        wire_vertex_shader(),
        wire_fragment_shader());

    if (!wireShaderResult) {
        print_graphics_error("wire shader", wireShaderResult.error());
        return EXIT_FAILURE;
    }

    std::vector<DemoMesh> demos;
    demos.push_back(make_original_mesh());
    demos.push_back(make_shrink_fatten_mesh());
    demos.push_back(make_randomized_mesh());
    demos.push_back(make_extrude_inset_mesh());

    std::vector<VisualMesh> visualMeshes;
    visualMeshes.resize(demos.size());

    for (std::size_t i = 0; i < demos.size(); ++i) {
        if (!upload_visual_mesh(demos[i].mesh, visualMeshes[i])) {
            return EXIT_FAILURE;
        }

        std::cout
            << "[OK] " << demos[i].name
            << " | vertices=" << TopologyTraversal::vertices(demos[i].mesh).size()
            << " edges=" << TopologyTraversal::edges(demos[i].mesh).size()
            << " faces=" << TopologyTraversal::faces(demos[i].mesh).size()
            << '\n';
    }

    Renderer renderer;
    RenderScene scene;

    scene.reserve(demos.size() * 2);

    for (std::size_t i = 0; i < demos.size(); ++i) {
        scene.add_object(make_render_object(
            static_cast<std::uint64_t>(i * 2 + 1),
            demos[i].name + " Solid",
            visualMeshes[i].solid,
            solidShader,
            demos[i].position,
            false));

        scene.add_object(make_render_object(
            static_cast<std::uint64_t>(i * 2 + 2),
            demos[i].name + " Wire",
            visualMeshes[i].wire,
            wireShader,
            demos[i].position + glm::vec3{ 0.0f, 0.01f, 0.0f },
            true));
    }

    std::cout << "[OK] Cena visual criada\n";
    std::cout << "Objetos: Original | Shrink/Fatten | Randomize | Extrude + Inset\n";
    std::cout << "Feche a janela para encerrar o teste.\n";

    while (!window.should_close()) {
        window.poll_events();

        viewport.sync_with_window(window);
        viewport.begin_frame();

        renderer.set_view_matrix(viewport.camera().view_matrix());
        renderer.set_projection_matrix(viewport.camera().projection_matrix());
        renderer.render(scene);

        window.swap_buffers();
    }

    context.shutdown();
    window.destroy();

    return EXIT_SUCCESS;
}