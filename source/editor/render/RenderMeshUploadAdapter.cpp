/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/RenderMeshUploadAdapter.h"

#include "kernel/geometry/render/RenderMesh.h"

namespace locus::editor {

    namespace {

        [[nodiscard]] graphics::MeshVertex make_mesh_vertex(
            const kernel::geometry::RenderVertex& vertex,
            const graphics::ColorRGBA& color)
        {
            graphics::MeshVertex meshVertex{};

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

            return meshVertex;
        }

        void copy_vertices(
            const kernel::geometry::RenderMesh& mesh,
            const RenderMeshUploadOptions& options,
            graphics::MeshUploadData& uploadData)
        {
            uploadData.vertices.reserve(mesh.vertices.size());

            for (const kernel::geometry::RenderVertex& vertex : mesh.vertices) {
                uploadData.vertices.push_back(make_mesh_vertex(vertex, options.color));
            }
        }

    } // namespace

    graphics::MeshUploadData RenderMeshUploadAdapter::build_triangle_upload_data(
        const kernel::geometry::RenderMesh& mesh,
        const RenderMeshUploadOptions& options,
        RenderMeshUploadResult* result)
    {
        RenderMeshUploadResult localResult{};

        graphics::MeshUploadData uploadData{};
        uploadData.topology = options.triangleTopology;
        uploadData.usage = options.usage;

        copy_vertices(mesh, options, uploadData);

        uploadData.indices.reserve(mesh.triangles.size() * 3U);

        for (const kernel::geometry::RenderTriangle& triangle : mesh.triangles) {
            uploadData.indices.push_back(static_cast<graphics::u32>(triangle.a));
            uploadData.indices.push_back(static_cast<graphics::u32>(triangle.b));
            uploadData.indices.push_back(static_cast<graphics::u32>(triangle.c));
        }

        localResult.vertexCount = mesh.vertices.size();
        localResult.triangleCount = mesh.triangles.size();
        localResult.lineCount = 0;
        localResult.indexCount = uploadData.indices.size();

        if (result) {
            *result = localResult;
        }

        return uploadData;
    }

    graphics::MeshUploadData RenderMeshUploadAdapter::build_line_upload_data(
        const kernel::geometry::RenderMesh& mesh,
        const RenderMeshUploadOptions& options,
        RenderMeshUploadResult* result)
    {
        RenderMeshUploadResult localResult{};

        graphics::MeshUploadData uploadData{};
        uploadData.topology = options.lineTopology;
        uploadData.usage = options.usage;

        copy_vertices(mesh, options, uploadData);

        uploadData.indices.reserve(mesh.lines.size() * 2U);

        for (const kernel::geometry::RenderLine& line : mesh.lines) {
            uploadData.indices.push_back(static_cast<graphics::u32>(line.a));
            uploadData.indices.push_back(static_cast<graphics::u32>(line.b));
        }

        localResult.vertexCount = mesh.vertices.size();
        localResult.triangleCount = 0;
        localResult.lineCount = mesh.lines.size();
        localResult.indexCount = uploadData.indices.size();

        if (result) {
            *result = localResult;
        }

        return uploadData;
    }

} // namespace locus::editor