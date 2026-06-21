/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/TopologyEditor.h"

#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

namespace locus::kernel::geometry {

    TopologyEditor::TopologyEditor(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    LEM& TopologyEditor::mesh()
    {
        return mesh_;
    }

    const LEM& TopologyEditor::mesh() const
    {
        return mesh_;
    }

    VertexHandle TopologyEditor::add_vertex(const glm::vec3& position)
    {
        VertexHandle handle = mesh_.add_vertex(position);

        if (mesh_.is_valid(handle)) {
            diff_.record(LEMChangeType::VertexAdded, handle);
        }

        return handle;
    }

    EdgeHandle TopologyEditor::find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB)
    {
        const std::size_t edgeCount = mesh_.edge_count();

        EdgeHandle handle = mesh_.find_or_create_edge(vertexA, vertexB);

        if (mesh_.is_valid(handle) && mesh_.edge_count() > edgeCount) {
            diff_.record(LEMChangeType::EdgeAdded, handle);
            diff_.record(LEMChangeType::VertexModified, vertexA);
            diff_.record(LEMChangeType::VertexModified, vertexB);
        }

        return handle;
    }

    FaceHandle TopologyEditor::add_face(const std::vector<VertexHandle>& vertices)
    {
        const std::size_t edgeCount = mesh_.edge_count();
        const std::size_t loopCount = mesh_.loop_count();
        const std::size_t faceCount = mesh_.face_count();

        FaceHandle handle = mesh_.add_face(vertices);

        if (!mesh_.is_valid(handle)) {
            return {};
        }

        for (std::size_t index = edgeCount; index < mesh_.edge_count(); ++index) {
            diff_.record(LEMChangeType::EdgeAdded, EdgeHandle(static_cast<IdValue>(index)));
        }

        for (std::size_t index = loopCount; index < mesh_.loop_count(); ++index) {
            diff_.record(LEMChangeType::LoopAdded, LoopHandle(static_cast<IdValue>(index)));
        }

        for (std::size_t index = faceCount; index < mesh_.face_count(); ++index) {
            diff_.record(LEMChangeType::FaceAdded, FaceHandle(static_cast<IdValue>(index)));
        }

        for (VertexHandle vertexHandle : vertices) {
            if (mesh_.is_valid(vertexHandle)) {
                diff_.record(LEMChangeType::VertexModified, vertexHandle);
            }
        }

        return handle;
    }

    void TopologyEditor::rebuild_face_normals()
    {
        NormalBuilder::rebuild_face_normals(mesh_);

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            diff_.record(LEMChangeType::NormalsChanged, handle);
        }
    }

    void TopologyEditor::clear()
    {
        mesh_.clear();
        diff_.record(LEMChangeType::MeshCleared, LEMElementType::Vertex, Id{});
    }

}