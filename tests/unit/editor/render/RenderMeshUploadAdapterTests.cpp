/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorRenderTestSuite.h"

#include "editor/render/RenderMeshUploadAdapter.h"
#include "kernel/geometry/render/RenderMesh.h"

#include <glm/vec3.hpp>

namespace {

[[nodiscard]] locus::kernel::geometry::RenderMesh make_render_mesh()
{
    locus::kernel::geometry::RenderMesh mesh;
    mesh.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 0.0f, 1.0f });
    mesh.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f });
    mesh.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 1.0f, 0.0f, 0.0f });
    mesh.add_triangle(0, 1, 2);
    mesh.add_line(2, 0);
    return mesh;
}

} // namespace

namespace locus::tests {

TestResult run_render_mesh_upload_adapter_tests()
{
    const kernel::geometry::RenderMesh mesh = make_render_mesh();

    editor::RenderMeshUploadOptions options;
    options.color = graphics::ColorRGBA{ 0.25f, 0.5f, 0.75f, 1.0f };
    options.usage = graphics::BufferUsage::Dynamic;
    options.triangleTopology = graphics::PrimitiveTopology::TriangleStrip;
    options.lineTopology = graphics::PrimitiveTopology::LineStrip;

    editor::RenderMeshUploadResult triangleResult;
    const graphics::MeshUploadData triangleUpload =
        editor::RenderMeshUploadAdapter::build_triangle_upload_data(
            mesh,
            options,
            &triangleResult);

    if (triangleUpload.vertices.size() != 3u ||
        triangleUpload.indices.size() != 3u ||
        triangleUpload.indices[0] != 0u ||
        triangleUpload.indices[1] != 1u ||
        triangleUpload.indices[2] != 2u ||
        triangleUpload.topology != graphics::PrimitiveTopology::TriangleStrip ||
        triangleUpload.usage != graphics::BufferUsage::Dynamic ||
        triangleUpload.vertices[1].position[0] != 1.0f ||
        triangleUpload.vertices[1].normal[1] != 1.0f ||
        triangleUpload.vertices[1].color[0] != 0.25f ||
        triangleUpload.vertices[1].color[1] != 0.5f ||
        triangleUpload.vertices[1].color[2] != 0.75f ||
        triangleUpload.vertices[1].color[3] != 1.0f) {
        return TestResult::fail("triangle upload should copy vertices, indices and options");
    }

    if (triangleResult.vertexCount != 3u ||
        triangleResult.triangleCount != 1u ||
        triangleResult.lineCount != 0u ||
        triangleResult.indexCount != 3u ||
        !triangleResult.has_triangles() ||
        triangleResult.has_lines()) {
        return TestResult::fail("triangle upload result should report triangle statistics");
    }

    editor::RenderMeshUploadResult lineResult;
    const graphics::MeshUploadData lineUpload =
        editor::RenderMeshUploadAdapter::build_line_upload_data(mesh, options, &lineResult);

    if (lineUpload.vertices.size() != 3u ||
        lineUpload.indices.size() != 2u ||
        lineUpload.indices[0] != 2u ||
        lineUpload.indices[1] != 0u ||
        lineUpload.topology != graphics::PrimitiveTopology::LineStrip ||
        lineUpload.usage != graphics::BufferUsage::Dynamic) {
        return TestResult::fail("line upload should copy vertices, line indices and options");
    }

    if (lineResult.vertexCount != 3u ||
        lineResult.triangleCount != 0u ||
        lineResult.lineCount != 1u ||
        lineResult.indexCount != 2u ||
        lineResult.has_triangles() ||
        !lineResult.has_lines()) {
        return TestResult::fail("line upload result should report line statistics");
    }

    editor::RenderMeshUploadResult emptyResult;
    const graphics::MeshUploadData emptyUpload =
        editor::RenderMeshUploadAdapter::build_triangle_upload_data(
            kernel::geometry::RenderMesh{},
            {},
            &emptyResult);

    if (!emptyUpload.is_empty() ||
        emptyResult.vertexCount != 0u ||
        emptyResult.indexCount != 0u ||
        emptyResult.has_triangles() ||
        emptyResult.has_lines()) {
        return TestResult::fail("empty render mesh should produce empty upload data and empty statistics");
    }

    return TestResult::pass();
}

} // namespace locus::tests
