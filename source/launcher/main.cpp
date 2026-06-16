#include "graphics/common/GraphicsConfig.h"
#include "graphics/gpu/RenderState.h"
#include "graphics/gpu/Shader.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploadData.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"
#include "graphics/window/Window.h"
#include "graphics/context/OpenGLContext.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/NormalBuilder.h"

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <string>
#include <vector>

namespace
{
    locus::kernel::geometry::LEM build_cube()
    {
        using namespace locus::kernel::geometry;

        LEM mesh;

        const VertexHandle v0 = mesh.add_vertex(glm::vec3{ -1.0f, -1.0f, -1.0f });
        const VertexHandle v1 = mesh.add_vertex(glm::vec3{ 1.0f, -1.0f, -1.0f });
        const VertexHandle v2 = mesh.add_vertex(glm::vec3{ 1.0f, 1.0f, -1.0f });
        const VertexHandle v3 = mesh.add_vertex(glm::vec3{ -1.0f, 1.0f, -1.0f });

        const VertexHandle v4 = mesh.add_vertex(glm::vec3{ -1.0f, -1.0f, 1.0f });
        const VertexHandle v5 = mesh.add_vertex(glm::vec3{ 1.0f, -1.0f, 1.0f });
        const VertexHandle v6 = mesh.add_vertex(glm::vec3{ 1.0f, 1.0f, 1.0f });
        const VertexHandle v7 = mesh.add_vertex(glm::vec3{ -1.0f, 1.0f, 1.0f });

        mesh.add_face(std::vector<VertexHandle>{ v0, v1, v2, v3 });
        mesh.add_face(std::vector<VertexHandle>{ v4, v7, v6, v5 });
        mesh.add_face(std::vector<VertexHandle>{ v0, v4, v5, v1 });
        mesh.add_face(std::vector<VertexHandle>{ v1, v5, v6, v2 });
        mesh.add_face(std::vector<VertexHandle>{ v2, v6, v7, v3 });
        mesh.add_face(std::vector<VertexHandle>{ v3, v7, v4, v0 });

        NormalBuilder::rebuild_face_normals(mesh);

        return mesh;
    }

    locus::graphics::MeshUploadData to_mesh_upload_data(const locus::kernel::geometry::RenderMesh& renderMesh)
    {
        locus::graphics::MeshUploadData uploadData;
        uploadData.topology = locus::graphics::PrimitiveTopology::Triangles;
        uploadData.usage = locus::graphics::BufferUsage::Static;

        uploadData.vertices.reserve(renderMesh.vertices.size());
        uploadData.indices.reserve(renderMesh.triangles.size() * 3);

        for (const locus::kernel::geometry::RenderVertex& renderVertex : renderMesh.vertices)
        {
            locus::graphics::MeshVertex vertex{};

            vertex.position[0] = renderVertex.position.x;
            vertex.position[1] = renderVertex.position.y;
            vertex.position[2] = renderVertex.position.z;

            vertex.normal[0] = renderVertex.normal.x;
            vertex.normal[1] = renderVertex.normal.y;
            vertex.normal[2] = renderVertex.normal.z;

            vertex.color[0] = 0.25f;
            vertex.color[1] = 0.72f;
            vertex.color[2] = 1.0f;
            vertex.color[3] = 1.0f;

            uploadData.vertices.push_back(vertex);
        }

        for (const locus::kernel::geometry::RenderTriangle& triangle : renderMesh.triangles)
        {
            uploadData.indices.push_back(triangle.a);
            uploadData.indices.push_back(triangle.b);
            uploadData.indices.push_back(triangle.c);
        }

        return uploadData;
    }

    std::string vertex_shader_source()
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

    std::string fragment_shader_source()
    {
        return R"(
#version 450 core

in vec3 v_Normal;
in vec4 v_Color;

uniform vec4 u_BaseColor;
uniform int u_UseVertexColor;
uniform vec3 u_LightDirection;

out vec4 FragColor;

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 lightDirection = normalize(-u_LightDirection);
    float diffuse = max(dot(normal, lightDirection), 0.0);
    float lighting = 0.35 + diffuse * 0.65;

    vec4 baseColor = u_UseVertexColor == 1 ? v_Color : u_BaseColor;
    FragColor = vec4(baseColor.rgb * lighting, baseColor.a);
}
)";
    }
}

int main()
{
    using namespace locus::graphics;
    using namespace locus::kernel::geometry;

    WindowCreateInfo windowInfo{};
    windowInfo.width = 1280;
    windowInfo.height = 720;
    windowInfo.title = "Locus3D Kernel Render Test";
    windowInfo.resizable = true;
    windowInfo.visible = true;
    windowInfo.decorated = true;
    windowInfo.requestOpenGLContext = true;
    windowInfo.openglMajorVersion = 4;
    windowInfo.openglMinorVersion = 5;
    windowInfo.openglCoreProfile = true;
    windowInfo.openglForwardCompatible = true;
    windowInfo.openglDebugContext = true;

    Window window;
    auto windowResult = window.create(windowInfo);
    if (!windowResult)
    {
        std::cerr << "Window error: " << windowResult.error().message << "\n";
        return 1;
    }

    GraphicsConfig graphicsConfig{};
    graphicsConfig.requestedMajorVersion = 4;
    graphicsConfig.requestedMinorVersion = 5;
    graphicsConfig.enableDebugOutput = true;
    graphicsConfig.enableVSync = true;

    OpenGLContext context;
    auto contextResult = context.initialize(window, graphicsConfig);
    if (!contextResult)
    {
        std::cerr << "Context error: " << contextResult.error().message << "\n";
        return 1;
    }

    context.set_vsync(true);

    Shader shader;
    auto shaderResult = shader.create_from_source(vertex_shader_source(), fragment_shader_source());
    if (!shaderResult)
    {
        std::cerr << "Shader error: " << shaderResult.error().message << "\n";
        return 1;
    }

    LEM lem = build_cube();

    RenderMesh renderMesh = MeshTriangulator::triangulate(lem);
    NormalBuilder::rebuild_normals(renderMesh, NormalBuildMode::Flat);

    MeshUploadData uploadData = to_mesh_upload_data(renderMesh);

    MeshUploader uploader;
    auto uploadResult = uploader.upload(uploadData);
    if (!uploadResult)
    {
        std::cerr << "Mesh upload error: " << uploadResult.error().message << "\n";
        return 1;
    }

    GpuMesh gpuMesh = uploadResult.move_value();

    RenderObject object{};
    object.id = 1;
    object.name = "LEM Cube";
    object.mesh = &gpuMesh;
    object.shader = &shader;
    object.layer = RenderLayer::Default;
    object.transform.position = glm::vec3{ 0.0f, 0.0f, 0.0f };
    object.transform.scale = glm::vec3{ 1.0f, 1.0f, 1.0f };

    RenderScene scene;
    scene.add_object(object);

    Renderer renderer;

    RenderState::reset_default();
    RenderState::set_depth_test(true);
    RenderState::set_cull_face(false);

    while (!window.should_close())
    {
        window.poll_events();

        const int framebufferWidth = window.framebuffer_width();
        const int framebufferHeight = window.framebuffer_height();

        RenderState::set_viewport(0, 0, framebufferWidth, framebufferHeight);

        glClearColor(0.08f, 0.08f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const float aspect = framebufferHeight > 0
            ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight)
            : 1.0f;

        const glm::mat4 view = glm::lookAt(
            glm::vec3{ 3.0f, 2.4f, 4.0f },
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 1.0f, 0.0f }
        );

        const glm::mat4 projection = glm::perspective(
            glm::radians(45.0f),
            aspect,
            0.1f,
            100.0f
        );

        renderer.set_view_matrix(view);
        renderer.set_projection_matrix(projection);
        renderer.render(scene);

        window.swap_buffers();
    }

    context.shutdown();
    return 0;
}