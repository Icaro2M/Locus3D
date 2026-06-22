/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/topology/TopologyCreation.h"

#include "kernel/geometry/mesh/LEM.h"

namespace locus::kernel::geometry::editing::topology {

    VertexHandle add_vertex(LEM& mesh, LEMDiff& diff, const glm::vec3& position)
    {
        VertexHandle handle = mesh.add_vertex(position);

        if (mesh.is_valid(handle)) {
            diff.record(LEMChangeType::VertexAdded, handle);
        }

        return handle;
    }

    EdgeHandle find_or_create_edge(LEM& mesh, LEMDiff& diff, VertexHandle vertexA, VertexHandle vertexB)
    {
        const std::size_t edgeCount = mesh.edge_count();

        EdgeHandle handle = mesh.find_or_create_edge(vertexA, vertexB);

        if (mesh.is_valid(handle) && mesh.edge_count() > edgeCount) {
            diff.record(LEMChangeType::EdgeAdded, handle);

            if (mesh.is_valid(vertexA)) {
                diff.record(LEMChangeType::VertexModified, vertexA);
            }

            if (mesh.is_valid(vertexB)) {
                diff.record(LEMChangeType::VertexModified, vertexB);
            }
        }

        return handle;
    }

    FaceHandle add_face(LEM& mesh, LEMDiff& diff, const std::vector<VertexHandle>& vertices)
    {
        const std::size_t edgeCount = mesh.edge_count();
        const std::size_t loopCount = mesh.loop_count();
        const std::size_t faceCount = mesh.face_count();

        FaceHandle handle = mesh.add_face(vertices);

        if (!mesh.is_valid(handle)) {
            return {};
        }

        for (std::size_t index = edgeCount; index < mesh.edge_count(); ++index) {
            diff.record(LEMChangeType::EdgeAdded, EdgeHandle(index));
        }

        for (std::size_t index = loopCount; index < mesh.loop_count(); ++index) {
            diff.record(LEMChangeType::LoopAdded, LoopHandle(index));
        }

        for (std::size_t index = faceCount; index < mesh.face_count(); ++index) {
            diff.record(LEMChangeType::FaceAdded, FaceHandle(index));
        }

        for (VertexHandle vertexHandle : vertices) {
            if (mesh.is_valid(vertexHandle)) {
                diff.record(LEMChangeType::VertexModified, vertexHandle);
            }
        }

        return handle;
    }

}