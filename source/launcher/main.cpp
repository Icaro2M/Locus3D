/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/RenderMeshUploadAdapter.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/RenderMesh.h"

#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace {

    constexpr float Epsilon = 0.0001f;

    bool nearly_equal(float lhs, float rhs, float epsilon = Epsilon)
    {
        return std::abs(lhs - rhs) <= epsilon;
    }

    bool expect(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return true;
        }

        std::cout << "[FAIL] " << message << '\n';
        return false;
    }

    bool expect_size(
        std::size_t actual,
        std::size_t expected,
        const std::string& message)
    {
        if (actual == expected) {
            std::cout << "[OK] " << message << " = " << actual << '\n';
            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << actual
            << " expected=" << expected << '\n';

        return false;
    }

    bool expect_float(
        float actual,
        float expected,
        const std::string& message)
    {
        if (nearly_equal(actual, expected)) {
            std::cout
                << "[OK] " << message
                << " = " << std::fixed << std::setprecision(4) << actual << '\n';
            return true;
        }

        std::cout
            << "[FAIL] " << message
            << " | actual=" << std::fixed << std::setprecision(4) << actual
            << " expected=" << expected << '\n';

        return false;
    }

    void print_render_mesh_summary(const locus::kernel::geometry::RenderMesh& renderMesh)
    {
        std::cout
            << "RenderMesh"
            << " | vertices: " << renderMesh.vertex_count()
            << " | triangles: " << renderMesh.triangle_count()
            << " | lines: " << renderMesh.line_count()
            << " | empty: " << (renderMesh.empty() ? "true" : "false")
            << '\n';
    }

    void print_upload_summary(
        const locus::graphics::MeshUploadData& uploadData,
        const locus::editor::RenderMeshUploadResult& result)
    {
        std::cout
            << "MeshUploadData"
            << " | vertices: " << uploadData.vertices.size()
            << " | indices: " << uploadData.indices.size()
            << " | result vertices: " << result.vertexCount
            << " | result triangles: " << result.triangleCount
            << " | result lines: " << result.lineCount
            << " | result indices: " << result.indexCount
            << '\n';
    }

    locus::kernel::geometry::FaceHandle make_triangle(
        locus::kernel::geometry::LEMEditor& editor)
    {
        const auto v0 = editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const auto v1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const auto v2 = editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });

        return editor.add_face(std::vector<locus::kernel::geometry::VertexHandle>{
            v0,
                v1,
                v2,
        });
    }

    locus::kernel::geometry::FaceHandle make_quad(
        locus::kernel::geometry::LEMEditor& editor)
    {
        const auto v0 = editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const auto v1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const auto v2 = editor.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });
        const auto v3 = editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });

        return editor.add_face(std::vector<locus::kernel::geometry::VertexHandle>{
            v0,
                v1,
                v2,
                v3,
        });
    }

    locus::kernel::geometry::FaceHandle make_concave_pentagon(
        locus::kernel::geometry::LEMEditor& editor)
    {
        const auto v0 = editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const auto v1 = editor.add_vertex(glm::vec3{ 2.0f, 0.0f, 0.0f });
        const auto v2 = editor.add_vertex(glm::vec3{ 2.0f, 2.0f, 0.0f });
        const auto v3 = editor.add_vertex(glm::vec3{ 1.0f, 0.75f, 0.0f });
        const auto v4 = editor.add_vertex(glm::vec3{ 0.0f, 2.0f, 0.0f });

        return editor.add_face(std::vector<locus::kernel::geometry::VertexHandle>{
            v0,
                v1,
                v2,
                v3,
                v4,
        });
    }

    bool test_empty_mesh()
    {
        using namespace locus;

        std::cout << "\n=== RenderMeshUploadAdapter: malha vazia ===\n";

        bool ok = true;

        kernel::geometry::LEM mesh{};
        const kernel::geometry::RenderMesh renderMesh =
            kernel::geometry::MeshTriangulator::triangulate(mesh);

        editor::RenderMeshUploadResult result{};
        const graphics::MeshUploadData uploadData =
            editor::RenderMeshUploadAdapter::build_triangle_upload_data(renderMesh, {}, &result);

        print_render_mesh_summary(renderMesh);
        print_upload_summary(uploadData, result);

        ok &= expect(renderMesh.empty(), "RenderMesh ficou vazio");
        ok &= expect(uploadData.is_empty(), "MeshUploadData ficou vazio");
        ok &= expect(!uploadData.has_indices(), "MeshUploadData nao tem indices");

        ok &= expect_size(result.vertexCount, 0, "result.vertexCount");
        ok &= expect_size(result.triangleCount, 0, "result.triangleCount");
        ok &= expect_size(result.lineCount, 0, "result.lineCount");
        ok &= expect_size(result.indexCount, 0, "result.indexCount");
        ok &= expect(!result.has_triangles(), "result.has_triangles() retornou false");

        return ok;
    }

    bool test_triangle()
    {
        using namespace locus;

        std::cout << "\n=== RenderMeshUploadAdapter: triangulo ===\n";

        bool ok = true;

        kernel::geometry::LEM mesh{};
        kernel::geometry::LEMEditor editorFacade{ mesh };

        const auto face = make_triangle(editorFacade);
        ok &= expect(mesh.is_valid(face), "face triangular criada");

        const kernel::geometry::RenderMesh renderMesh =
            kernel::geometry::MeshTriangulator::triangulate(mesh);

        editor::RenderMeshUploadResult result{};
        const graphics::MeshUploadData uploadData =
            editor::RenderMeshUploadAdapter::build_triangle_upload_data(renderMesh, {}, &result);

        print_render_mesh_summary(renderMesh);
        print_upload_summary(uploadData, result);

        ok &= expect(!renderMesh.empty(), "RenderMesh nao ficou vazio");
        ok &= expect(!uploadData.is_empty(), "MeshUploadData nao ficou vazio");
        ok &= expect(uploadData.has_indices(), "MeshUploadData tem indices");
        ok &= expect(uploadData.topology == graphics::PrimitiveTopology::Triangles, "topologia ficou Triangles");
        ok &= expect(uploadData.usage == graphics::BufferUsage::Static, "usage default ficou Static");

        ok &= expect_size(renderMesh.vertex_count(), 3, "RenderMesh vertices");
        ok &= expect_size(renderMesh.triangle_count(), 1, "RenderMesh triangles");

        ok &= expect_size(uploadData.vertices.size(), 3, "upload vertices");
        ok &= expect_size(uploadData.indices.size(), 3, "upload indices");

        ok &= expect_size(result.vertexCount, 3, "result.vertexCount");
        ok &= expect_size(result.triangleCount, 1, "result.triangleCount");
        ok &= expect_size(result.lineCount, 0, "result.lineCount");
        ok &= expect_size(result.indexCount, 3, "result.indexCount");
        ok &= expect(result.has_triangles(), "result.has_triangles() retornou true");

        ok &= expect_float(uploadData.vertices[0].position[0], 0.0f, "v0.position.x");
        ok &= expect_float(uploadData.vertices[1].position[0], 1.0f, "v1.position.x");
        ok &= expect_float(uploadData.vertices[2].position[1], 1.0f, "v2.position.y");

        return ok;
    }

    bool test_quad()
    {
        using namespace locus;

        std::cout << "\n=== RenderMeshUploadAdapter: quad ===\n";

        bool ok = true;

        kernel::geometry::LEM mesh{};
        kernel::geometry::LEMEditor editorFacade{ mesh };

        const auto face = make_quad(editorFacade);
        ok &= expect(mesh.is_valid(face), "face quad criada");

        const kernel::geometry::RenderMesh renderMesh =
            kernel::geometry::MeshTriangulator::triangulate(mesh);

        editor::RenderMeshUploadResult result{};
        const graphics::MeshUploadData uploadData =
            editor::RenderMeshUploadAdapter::build_triangle_upload_data(renderMesh, {}, &result);

        print_render_mesh_summary(renderMesh);
        print_upload_summary(uploadData, result);

        ok &= expect_size(renderMesh.vertex_count(), 4, "RenderMesh vertices");
        ok &= expect_size(renderMesh.triangle_count(), 2, "RenderMesh triangles");

        ok &= expect_size(uploadData.vertices.size(), 4, "upload vertices");
        ok &= expect_size(uploadData.indices.size(), 6, "upload indices");

        ok &= expect_size(result.vertexCount, 4, "result.vertexCount");
        ok &= expect_size(result.triangleCount, 2, "result.triangleCount");
        ok &= expect_size(result.indexCount, 6, "result.indexCount");

        return ok;
    }

    bool test_concave_pentagon()
    {
        using namespace locus;

        std::cout << "\n=== RenderMeshUploadAdapter: pentagono concavo ===\n";

        bool ok = true;

        kernel::geometry::LEM mesh{};
        kernel::geometry::LEMEditor editorFacade{ mesh };

        const auto face = make_concave_pentagon(editorFacade);
        ok &= expect(mesh.is_valid(face), "face pentagonal concava criada");

        const kernel::geometry::RenderMesh renderMesh =
            kernel::geometry::MeshTriangulator::triangulate(mesh);

        editor::RenderMeshUploadResult result{};
        const graphics::MeshUploadData uploadData =
            editor::RenderMeshUploadAdapter::build_triangle_upload_data(renderMesh, {}, &result);

        print_render_mesh_summary(renderMesh);
        print_upload_summary(uploadData, result);

        ok &= expect_size(renderMesh.vertex_count(), 5, "RenderMesh vertices");
        ok &= expect_size(renderMesh.triangle_count(), 3, "RenderMesh triangles");

        ok &= expect_size(uploadData.vertices.size(), 5, "upload vertices");
        ok &= expect_size(uploadData.indices.size(), 9, "upload indices");

        ok &= expect_size(result.vertexCount, 5, "result.vertexCount");
        ok &= expect_size(result.triangleCount, 3, "result.triangleCount");
        ok &= expect_size(result.indexCount, 9, "result.indexCount");

        return ok;
    }

    bool test_manual_line_upload()
    {
        using namespace locus;

        std::cout << "\n=== RenderMeshUploadAdapter: linhas manuais ===\n";

        bool ok = true;

        kernel::geometry::RenderMesh renderMesh{};

        const auto a = renderMesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const auto b = renderMesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const auto c = renderMesh.add_vertex(glm::vec3{ 1.0f, 1.0f, 0.0f });

        renderMesh.add_line(a, b);
        renderMesh.add_line(b, c);

        editor::RenderMeshUploadResult result{};
        const graphics::MeshUploadData uploadData =
            editor::RenderMeshUploadAdapter::build_line_upload_data(renderMesh, {}, &result);

        print_render_mesh_summary(renderMesh);
        print_upload_summary(uploadData, result);

        ok &= expect(!renderMesh.empty(), "RenderMesh de linhas nao ficou vazio");
        ok &= expect(!uploadData.is_empty(), "MeshUploadData de linhas nao ficou vazio");
        ok &= expect(uploadData.has_indices(), "MeshUploadData de linhas tem indices");
        ok &= expect(uploadData.topology == graphics::PrimitiveTopology::Lines, "topologia ficou Lines");

        ok &= expect_size(renderMesh.vertex_count(), 3, "RenderMesh vertices");
        ok &= expect_size(renderMesh.line_count(), 2, "RenderMesh lines");

        ok &= expect_size(uploadData.vertices.size(), 3, "upload vertices");
        ok &= expect_size(uploadData.indices.size(), 4, "upload indices");

        ok &= expect_size(result.vertexCount, 3, "result.vertexCount");
        ok &= expect_size(result.triangleCount, 0, "result.triangleCount");
        ok &= expect_size(result.lineCount, 2, "result.lineCount");
        ok &= expect_size(result.indexCount, 4, "result.indexCount");
        ok &= expect(result.has_lines(), "result.has_lines() retornou true");

        return ok;
    }

    bool test_color_and_usage()
    {
        using namespace locus;

        std::cout << "\n=== RenderMeshUploadAdapter: cor e usage ===\n";

        bool ok = true;

        kernel::geometry::LEM mesh{};
        kernel::geometry::LEMEditor editorFacade{ mesh };

        const auto face = make_triangle(editorFacade);
        ok &= expect(mesh.is_valid(face), "face triangular criada");

        const kernel::geometry::RenderMesh renderMesh =
            kernel::geometry::MeshTriangulator::triangulate(mesh);

        editor::RenderMeshUploadOptions options{};
        options.color = graphics::ColorRGBA{ 0.25f, 0.50f, 0.75f, 1.0f };
        options.usage = graphics::BufferUsage::Dynamic;

        editor::RenderMeshUploadResult result{};
        const graphics::MeshUploadData uploadData =
            editor::RenderMeshUploadAdapter::build_triangle_upload_data(renderMesh, options, &result);

        print_render_mesh_summary(renderMesh);
        print_upload_summary(uploadData, result);

        ok &= expect(uploadData.usage == graphics::BufferUsage::Dynamic, "usage ficou Dynamic");
        ok &= expect_size(uploadData.vertices.size(), 3, "upload vertices");

        ok &= expect_float(uploadData.vertices[0].color[0], 0.25f, "color.r");
        ok &= expect_float(uploadData.vertices[0].color[1], 0.50f, "color.g");
        ok &= expect_float(uploadData.vertices[0].color[2], 0.75f, "color.b");
        ok &= expect_float(uploadData.vertices[0].color[3], 1.00f, "color.a");

        return ok;
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor RenderMeshUploadAdapter Smoke Test ===\n";

    bool ok = true;

    ok &= test_empty_mesh();
    ok &= test_triangle();
    ok &= test_quad();
    ok &= test_concave_pentagon();
    ok &= test_manual_line_upload();
    ok &= test_color_and_usage();

    std::cout << "\n=== Resultado final ===\n";

    if (ok) {
        std::cout << "[OK] Todos os testes de RenderMeshUploadAdapter passaram.\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] Algum teste de RenderMeshUploadAdapter falhou.\n";
    return EXIT_FAILURE;
}