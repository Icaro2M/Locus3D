/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/topology/DeleteMeshElementsOp.h"

#include "kernel/common/Error.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <string>
#include <utility>

namespace locus::kernel::modeling {

    namespace {

        template <typename Handle>
        [[nodiscard]] bool contains(
            const std::vector<Handle>& handles,
            Handle handle)
        {
            return std::find(handles.begin(), handles.end(), handle)
                != handles.end();
        }

        template <typename Handle>
        void append_unique(std::vector<Handle>& handles, Handle handle)
        {
            if (!contains(handles, handle)) {
                handles.push_back(handle);
            }
        }

        template <typename Handle>
        [[nodiscard]] bool validate_targets(
            const geometry::LEM& mesh,
            const std::vector<Handle>& handles)
        {
            if (handles.empty()) {
                return false;
            }

            for (const Handle handle : handles) {
                if (!mesh.is_valid(handle)) {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] std::vector<geometry::FaceHandle>
            unique_valid_faces(
                const geometry::LEM& mesh,
                const std::vector<geometry::FaceHandle>& faces)
        {
            std::vector<geometry::FaceHandle> result{};
            result.reserve(faces.size());

            for (const geometry::FaceHandle face : faces) {
                if (mesh.is_valid(face)) {
                    append_unique(result, face);
                }
            }

            return result;
        }

        [[nodiscard]] std::vector<geometry::EdgeHandle>
            unique_valid_edges(
                const geometry::LEM& mesh,
                const std::vector<geometry::EdgeHandle>& edges)
        {
            std::vector<geometry::EdgeHandle> result{};
            result.reserve(edges.size());

            for (const geometry::EdgeHandle edge : edges) {
                if (mesh.is_valid(edge)) {
                    append_unique(result, edge);
                }
            }

            return result;
        }

        [[nodiscard]] std::vector<geometry::VertexHandle>
            unique_valid_vertices(
                const geometry::LEM& mesh,
                const std::vector<geometry::VertexHandle>& vertices)
        {
            std::vector<geometry::VertexHandle> result{};
            result.reserve(vertices.size());

            for (const geometry::VertexHandle vertex : vertices) {
                if (mesh.is_valid(vertex)) {
                    append_unique(result, vertex);
                }
            }

            return result;
        }

        [[nodiscard]] std::vector<geometry::FaceHandle>
            collect_edge_faces(
                const geometry::LEM& mesh,
                const std::vector<geometry::EdgeHandle>& edges)
        {
            std::vector<geometry::FaceHandle> result{};

            for (const geometry::EdgeHandle edge : edges) {
                for (const geometry::FaceHandle face
                    : geometry::TopologyTraversal::edge_faces(mesh, edge)) {
                    if (mesh.is_valid(face)) {
                        append_unique(result, face);
                    }
                }
            }

            return result;
        }

        [[nodiscard]] std::vector<geometry::FaceHandle>
            collect_vertex_faces(
                const geometry::LEM& mesh,
                const std::vector<geometry::VertexHandle>& vertices)
        {
            std::vector<geometry::FaceHandle> result{};

            for (const geometry::FaceHandle face
                : geometry::TopologyTraversal::faces(mesh)) {
                for (const geometry::VertexHandle vertex
                    : geometry::TopologyTraversal::face_vertices(mesh, face)) {
                    if (contains(vertices, vertex)) {
                        append_unique(result, face);
                        break;
                    }
                }
            }

            return result;
        }

        [[nodiscard]] std::vector<geometry::EdgeHandle>
            collect_vertex_edges(
                const geometry::LEM& mesh,
                const std::vector<geometry::VertexHandle>& vertices)
        {
            std::vector<geometry::EdgeHandle> result{};

            for (const geometry::EdgeHandle edge
                : geometry::TopologyTraversal::edges(mesh)) {
                const geometry::Edge& element = mesh.edge(edge);

                if (contains(vertices, element.vertexA)
                    || contains(vertices, element.vertexB)) {
                    append_unique(result, edge);
                }
            }

            return result;
        }

        [[nodiscard]] OperationResult invalid_targets_result(
            const char* label)
        {
            return OperationResult::fail(
                kernel::ErrorCode::InvalidArgument,
                std::string{ "Delete operation received invalid " }
                + label
                + " handles.");
        }

    } // namespace

    DeleteMeshElementsOp DeleteMeshElementsOp::vertices(
        std::vector<geometry::VertexHandle> vertices)
    {
        DeleteMeshElementsOp operation{ DeleteMeshElementMode::Vertices };
        operation.vertices_ = std::move(vertices);
        return operation;
    }

    DeleteMeshElementsOp DeleteMeshElementsOp::edges(
        std::vector<geometry::EdgeHandle> edges)
    {
        DeleteMeshElementsOp operation{ DeleteMeshElementMode::Edges };
        operation.edges_ = std::move(edges);
        return operation;
    }

    DeleteMeshElementsOp DeleteMeshElementsOp::faces(
        std::vector<geometry::FaceHandle> faces)
    {
        DeleteMeshElementsOp operation{ DeleteMeshElementMode::Faces };
        operation.faces_ = std::move(faces);
        return operation;
    }

    std::string_view DeleteMeshElementsOp::name() const
    {
        return "DeleteMeshElementsOp";
    }

    DeleteMeshElementMode DeleteMeshElementsOp::mode() const
    {
        return mode_;
    }

    const std::vector<geometry::VertexHandle>&
        DeleteMeshElementsOp::vertices() const
    {
        return vertices_;
    }

    const std::vector<geometry::EdgeHandle>&
        DeleteMeshElementsOp::edges() const
    {
        return edges_;
    }

    const std::vector<geometry::FaceHandle>&
        DeleteMeshElementsOp::faces() const
    {
        return faces_;
    }

    DeleteMeshElementsOp::DeleteMeshElementsOp(
        DeleteMeshElementMode mode)
        : mode_(mode)
    {
    }

    OperationResult DeleteMeshElementsOp::execute_impl(
        OperationContext& context)
    {
        switch (mode_) {
        case DeleteMeshElementMode::Vertices:
            return execute_vertices(context);

        case DeleteMeshElementMode::Edges:
            return execute_edges(context);

        case DeleteMeshElementMode::Faces:
            return execute_faces(context);
        }

        return OperationResult::no_change(
            "Delete operation has no valid target mode.");
    }

    OperationResult DeleteMeshElementsOp::execute_faces(
        OperationContext& context) const
    {
        geometry::LEM& mesh = context.editable_mesh();

        if (!validate_targets(mesh, faces_)) {
            return invalid_targets_result("face");
        }

        const std::vector<geometry::FaceHandle> targets =
            unique_valid_faces(mesh, faces_);

        geometry::LEMEditor editor(mesh);
        std::size_t removedCount = 0u;

        for (const geometry::FaceHandle face : targets) {
            if (mesh.is_valid(face) && editor.remove_face(face)) {
                ++removedCount;
            }
        }

        if (removedCount == 0u) {
            return OperationResult::no_change(
                "Delete operation did not remove any face.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    OperationResult DeleteMeshElementsOp::execute_edges(
        OperationContext& context) const
    {
        geometry::LEM& mesh = context.editable_mesh();

        if (!validate_targets(mesh, edges_)) {
            return invalid_targets_result("edge");
        }

        const std::vector<geometry::EdgeHandle> targetEdges =
            unique_valid_edges(mesh, edges_);
        const std::vector<geometry::FaceHandle> incidentFaces =
            collect_edge_faces(mesh, targetEdges);

        geometry::LEMEditor editor(mesh);

        for (const geometry::FaceHandle face : incidentFaces) {
            if (mesh.is_valid(face) && !editor.remove_face(face)) {
                return OperationResult::fail(
                    kernel::ErrorCode::InvalidState,
                    "Delete operation failed to remove an incident face.");
            }
        }

        std::size_t removedEdges = 0u;

        for (const geometry::EdgeHandle edge : targetEdges) {
            if (mesh.is_valid(edge)
                && editor.remove_edge_if_loose(edge)) {
                ++removedEdges;
            }
        }

        if (removedEdges == 0u) {
            return OperationResult::no_change(
                "Delete operation did not remove any edge.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    OperationResult DeleteMeshElementsOp::execute_vertices(
        OperationContext& context) const
    {
        geometry::LEM& mesh = context.editable_mesh();

        if (!validate_targets(mesh, vertices_)) {
            return invalid_targets_result("vertex");
        }

        const std::vector<geometry::VertexHandle> targetVertices =
            unique_valid_vertices(mesh, vertices_);
        const std::vector<geometry::FaceHandle> incidentFaces =
            collect_vertex_faces(mesh, targetVertices);
        const std::vector<geometry::EdgeHandle> incidentEdges =
            collect_vertex_edges(mesh, targetVertices);

        geometry::LEMEditor editor(mesh);

        for (const geometry::FaceHandle face : incidentFaces) {
            if (mesh.is_valid(face) && !editor.remove_face(face)) {
                return OperationResult::fail(
                    kernel::ErrorCode::InvalidState,
                    "Delete operation failed to remove an incident face.");
            }
        }

        for (const geometry::EdgeHandle edge : incidentEdges) {
            if (mesh.is_valid(edge)
                && !editor.remove_edge_if_loose(edge)) {
                return OperationResult::fail(
                    kernel::ErrorCode::InvalidState,
                    "Delete operation failed to remove an incident edge.");
            }
        }

        std::size_t removedVertices = 0u;

        for (const geometry::VertexHandle vertex : targetVertices) {
            if (mesh.is_valid(vertex)
                && editor.remove_vertex_if_loose(vertex)) {
                ++removedVertices;
            }
        }

        if (removedVertices == 0u) {
            return OperationResult::no_change(
                "Delete operation did not remove any vertex.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

} // namespace locus::kernel::modeling
