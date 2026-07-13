/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/primitives/PrimitiveMeshConverter.h"

namespace locus::graphics {

    namespace {

        [[nodiscard]] MeshVertex to_mesh_vertex(
            const PrimitiveVertex& vertex
        ) {
            MeshVertex meshVertex{};

            meshVertex.position[0] = vertex.position.x;
            meshVertex.position[1] = vertex.position.y;
            meshVertex.position[2] = vertex.position.z;

            meshVertex.normal[0] = vertex.normal.x;
            meshVertex.normal[1] = vertex.normal.y;
            meshVertex.normal[2] = vertex.normal.z;

            meshVertex.color[0] = vertex.color.r;
            meshVertex.color[1] = vertex.color.g;
            meshVertex.color[2] = vertex.color.b;
            meshVertex.color[3] = vertex.color.a;

            return meshVertex;
        }

    } // namespace

    MeshUploadData PrimitiveMeshConverter::to_upload_data(
        const PrimitiveMesh& mesh,
        const BufferUsage usage
    ) {
        MeshUploadData uploadData{};

        uploadData.topology = mesh.topology;
        uploadData.usage = usage;
        uploadData.indices = mesh.indices;

        uploadData.vertices.reserve(mesh.vertices.size());

        for (const PrimitiveVertex& vertex : mesh.vertices) {
            uploadData.vertices.push_back(to_mesh_vertex(vertex));
        }

        return uploadData;
    }

} // namespace locus::graphics