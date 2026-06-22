/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/topology/SubdivideOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <utility>

namespace locus::kernel::modeling {

    SubdivideOp::SubdivideOp(geometry::EdgeHandle edge)
        : target_(SubdivideTarget::Edges)
        , edges_({ edge })
    {
    }

    SubdivideOp::SubdivideOp(std::vector<geometry::EdgeHandle> edges)
        : target_(SubdivideTarget::Edges)
        , edges_(std::move(edges))
    {
    }

    SubdivideOp SubdivideOp::faces(std::vector<geometry::FaceHandle> faces)
    {
        SubdivideOp op;
        op.set_target(SubdivideTarget::Faces);
        op.set_faces(std::move(faces));
        return op;
    }

    SubdivideOp SubdivideOp::face(geometry::FaceHandle face)
    {
        SubdivideOp op;
        op.set_target(SubdivideTarget::Faces);
        op.set_faces({ face });
        return op;
    }

    std::string_view SubdivideOp::name() const
    {
        return "SubdivideOp";
    }

    void SubdivideOp::set_target(SubdivideTarget target)
    {
        target_ = target;
    }

    SubdivideTarget SubdivideOp::target() const
    {
        return target_;
    }

    void SubdivideOp::set_edges(std::vector<geometry::EdgeHandle> edges)
    {
        edges_ = std::move(edges);
    }

    const std::vector<geometry::EdgeHandle>& SubdivideOp::edges() const
    {
        return edges_;
    }

    void SubdivideOp::clear_edges()
    {
        edges_.clear();
    }

    void SubdivideOp::set_faces(std::vector<geometry::FaceHandle> faces)
    {
        faces_ = std::move(faces);
    }

    const std::vector<geometry::FaceHandle>& SubdivideOp::faces() const
    {
        return faces_;
    }

    void SubdivideOp::clear_faces()
    {
        faces_.clear();
    }

    OperationResult SubdivideOp::execute_impl(OperationContext& context)
    {
        switch (target_) {
        case SubdivideTarget::Edges:
        case SubdivideTarget::SelectedEdges:
            return execute_edges(context);

        case SubdivideTarget::Faces:
        case SubdivideTarget::SelectedFaces:
            return execute_faces(context);
        }

        return OperationResult::no_change("Subdivide operation has no valid target mode.");
    }

    OperationResult SubdivideOp::execute_edges(OperationContext& context) const
    {
        geometry::LEM& mesh = context.editable_mesh();
        const std::vector<geometry::EdgeHandle> targets = collect_edges(mesh);

        if (targets.empty()) {
            return OperationResult::no_change(
                "Subdivide operation found no valid edges.");
        }

        geometry::LEMEditor editor(mesh);

        std::size_t splitCount = 0;

        for (geometry::EdgeHandle edge : targets) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (editor.split_edge_at_param(edge, 0.5f).has_value()) {
                ++splitCount;
            }
        }

        if (splitCount == 0) {
            return OperationResult::no_change(
                "Subdivide operation did not split any edge.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    OperationResult SubdivideOp::execute_faces(OperationContext& context) const
    {
        geometry::LEM& mesh = context.editable_mesh();
        const std::vector<geometry::FaceHandle> targets = collect_faces(mesh);

        if (targets.empty()) {
            return OperationResult::no_change(
                "Subdivide operation found no valid faces.");
        }

        geometry::LEMEditor editor(mesh);

        std::size_t subdividedCount = 0;

        for (geometry::FaceHandle face : targets) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            const std::vector<geometry::VertexHandle> vertices =
                geometry::TopologyTraversal::face_vertices(mesh, face);

            if (vertices.size() < 3) {
                continue;
            }

            glm::vec3 center{ 0.0f };

            bool allVerticesValid = true;

            for (geometry::VertexHandle vertex : vertices) {
                if (!mesh.is_valid(vertex)) {
                    allVerticesValid = false;
                    break;
                }

                center += mesh.vertex(vertex).position;
            }

            if (!allVerticesValid) {
                continue;
            }

            center /= static_cast<float>(vertices.size());

            if (!editor.remove_face(face)) {
                continue;
            }

            geometry::VertexHandle centerVertex = editor.add_vertex(center);

            if (!mesh.is_valid(centerVertex)) {
                continue;
            }

            std::size_t createdFaceCount = 0;

            for (std::size_t i = 0; i < vertices.size(); ++i) {
                const geometry::VertexHandle vertexA = vertices[i];
                const geometry::VertexHandle vertexB = vertices[(i + 1) % vertices.size()];

                if (!mesh.is_valid(vertexA) || !mesh.is_valid(vertexB)) {
                    continue;
                }

                const geometry::FaceHandle newFace =
                    editor.add_face({ vertexA, vertexB, centerVertex });

                if (mesh.is_valid(newFace)) {
                    ++createdFaceCount;
                }
            }

            if (createdFaceCount > 0) {
                ++subdividedCount;
            }
        }

        if (subdividedCount == 0) {
            return OperationResult::no_change(
                "Subdivide operation did not subdivide any face.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::EdgeHandle> SubdivideOp::collect_edges(
        const geometry::LEM& mesh) const
    {
        std::vector<geometry::EdgeHandle> result;

        if (!edges_.empty()) {
            result.reserve(edges_.size());

            for (geometry::EdgeHandle edge : edges_) {
                if (!mesh.is_valid(edge) || contains(result, edge)) {
                    continue;
                }

                result.push_back(edge);
            }

            return result;
        }

        const std::vector<geometry::EdgeHandle> activeEdges =
            geometry::TopologyTraversal::edges(mesh);

        result.reserve(activeEdges.size());

        for (geometry::EdgeHandle edge : activeEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (target_ == SubdivideTarget::SelectedEdges && !mesh.edge(edge).selected) {
                continue;
            }

            result.push_back(edge);
        }

        return result;
    }

    std::vector<geometry::FaceHandle> SubdivideOp::collect_faces(
        const geometry::LEM& mesh) const
    {
        std::vector<geometry::FaceHandle> result;

        if (!faces_.empty()) {
            result.reserve(faces_.size());

            for (geometry::FaceHandle face : faces_) {
                if (!mesh.is_valid(face) || contains(result, face)) {
                    continue;
                }

                result.push_back(face);
            }

            return result;
        }

        const std::vector<geometry::FaceHandle> activeFaces =
            geometry::TopologyTraversal::faces(mesh);

        result.reserve(activeFaces.size());

        for (geometry::FaceHandle face : activeFaces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            if (target_ == SubdivideTarget::SelectedFaces && !mesh.face(face).selected) {
                continue;
            }

            result.push_back(face);
        }

        return result;
    }

}