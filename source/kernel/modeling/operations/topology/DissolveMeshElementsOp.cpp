/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/topology/DissolveMeshElementsOp.h"

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

        template <typename Handle>
        [[nodiscard]] std::vector<Handle> unique_targets(
            const std::vector<Handle>& handles)
        {
            std::vector<Handle> result{};
            result.reserve(handles.size());

            for (const Handle handle : handles) {
                append_unique(result, handle);
            }

            std::sort(
                result.begin(),
                result.end(),
                [](const Handle lhs, const Handle rhs) {
                    return lhs.id.value < rhs.id.value;
                });

            return result;
        }

        [[nodiscard]] OperationResult invalid_targets_result(
            const char* label)
        {
            return OperationResult::fail(
                kernel::ErrorCode::InvalidArgument,
                std::string{ "Dissolve operation received invalid " }
                    + label
                    + " handles.");
        }

        [[nodiscard]] bool has_duplicate_vertices(
            const std::vector<geometry::VertexHandle>& vertices)
        {
            for (std::size_t index = 0u; index < vertices.size(); ++index) {
                for (std::size_t next = index + 1u;
                    next < vertices.size();
                    ++next) {
                    if (vertices[index] == vertices[next]) {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]] std::size_t index_of(
            const std::vector<geometry::VertexHandle>& vertices,
            geometry::VertexHandle vertex)
        {
            const auto iterator =
                std::find(vertices.begin(), vertices.end(), vertex);

            if (iterator == vertices.end()) {
                return vertices.size();
            }

            return static_cast<std::size_t>(
                std::distance(vertices.begin(), iterator));
        }

        [[nodiscard]] bool directed_edge_in_face(
            const std::vector<geometry::VertexHandle>& vertices,
            geometry::VertexHandle from,
            geometry::VertexHandle to)
        {
            const std::size_t fromIndex = index_of(vertices, from);

            return fromIndex < vertices.size()
                && vertices[(fromIndex + 1u) % vertices.size()] == to;
        }

        [[nodiscard]] std::vector<geometry::VertexHandle> path_forward(
            const std::vector<geometry::VertexHandle>& vertices,
            geometry::VertexHandle from,
            geometry::VertexHandle to)
        {
            std::vector<geometry::VertexHandle> result{};
            const std::size_t fromIndex = index_of(vertices, from);

            if (fromIndex >= vertices.size()) {
                return result;
            }

            for (std::size_t offset = 0u; offset < vertices.size(); ++offset) {
                const geometry::VertexHandle vertex =
                    vertices[(fromIndex + offset) % vertices.size()];
                result.push_back(vertex);

                if (vertex == to) {
                    return result;
                }
            }

            return {};
        }

        [[nodiscard]] std::vector<geometry::VertexHandle> merged_edge_face(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edge)
        {
            if (!mesh.is_valid(edge)) {
                return {};
            }

            const geometry::Edge& edgeElement = mesh.edge(edge);
            if (!mesh.is_valid(edgeElement.vertexA)
                || !mesh.is_valid(edgeElement.vertexB)) {
                return {};
            }

            const std::vector<geometry::FaceHandle> faces =
                geometry::TopologyTraversal::edge_faces(mesh, edge);

            if (faces.size() != 2u) {
                return {};
            }

            const geometry::VertexHandle vertexA = edgeElement.vertexA;
            const geometry::VertexHandle vertexB = edgeElement.vertexB;

            const std::vector<geometry::VertexHandle> first =
                geometry::TopologyTraversal::face_vertices(mesh, faces[0]);
            const std::vector<geometry::VertexHandle> second =
                geometry::TopologyTraversal::face_vertices(mesh, faces[1]);

            if (first.size() < 3u
                || second.size() < 3u
                || has_duplicate_vertices(first)
                || has_duplicate_vertices(second)) {
                return {};
            }

            const bool firstAB = directed_edge_in_face(first, vertexA, vertexB);
            const bool firstBA = directed_edge_in_face(first, vertexB, vertexA);
            const bool secondAB = directed_edge_in_face(second, vertexA, vertexB);
            const bool secondBA = directed_edge_in_face(second, vertexB, vertexA);

            if ((firstAB == firstBA)
                || (secondAB == secondBA)
                || firstAB == secondAB) {
                return {};
            }

            const std::vector<geometry::VertexHandle> firstPath =
                firstAB
                    ? path_forward(first, vertexB, vertexA)
                    : path_forward(first, vertexA, vertexB);
            const std::vector<geometry::VertexHandle> secondPath =
                firstAB
                    ? path_forward(second, vertexA, vertexB)
                    : path_forward(second, vertexB, vertexA);

            if (firstPath.size() < 2u || secondPath.size() < 2u) {
                return {};
            }

            std::vector<geometry::VertexHandle> merged = firstPath;
            merged.insert(
                merged.end(),
                secondPath.begin() + 1,
                secondPath.end() - 1);

            if (merged.size() < 3u || has_duplicate_vertices(merged)) {
                return {};
            }

            return merged;
        }

        [[nodiscard]] bool dissolve_one_edge(
            geometry::LEM& mesh,
            geometry::LEMEditor& editor,
            geometry::EdgeHandle edge)
        {
            const std::vector<geometry::FaceHandle> faces =
                geometry::TopologyTraversal::edge_faces(mesh, edge);
            const std::vector<geometry::VertexHandle> merged =
                merged_edge_face(mesh, edge);

            if (faces.size() != 2u || merged.empty()) {
                return false;
            }

            for (const geometry::FaceHandle face : faces) {
                if (!mesh.is_valid(face) || !editor.remove_face(face)) {
                    return false;
                }
            }

            if (!mesh.is_valid(edge) || !editor.remove_edge_if_loose(edge)) {
                return false;
            }

            const geometry::FaceHandle face = editor.add_face(merged);
            return mesh.is_valid(face);
        }

        [[nodiscard]] bool dissolve_one_vertex(
            geometry::LEM& mesh,
            geometry::LEMEditor& editor,
            geometry::VertexHandle vertex)
        {
            if (!mesh.is_valid(vertex)) {
                return false;
            }

            if (!geometry::TopologyTraversal::vertex_loops(mesh, vertex).empty()) {
                return false;
            }

            std::vector<geometry::EdgeHandle> incidentEdges{};

            for (const geometry::EdgeHandle edge
                : geometry::TopologyTraversal::edges(mesh)) {
                const geometry::Edge& element = mesh.edge(edge);

                if (element.vertexA == vertex
                    || element.vertexB == vertex) {
                    append_unique(incidentEdges, edge);
                }
            }

            if (incidentEdges.empty() || incidentEdges.size() > 2u) {
                return false;
            }

            std::vector<geometry::VertexHandle> neighbors{};

            for (const geometry::EdgeHandle edge : incidentEdges) {
                if (!mesh.is_valid(edge)) {
                    return false;
                }

                const geometry::Edge& element = mesh.edge(edge);
                const geometry::VertexHandle neighbor =
                    element.vertexA == vertex ? element.vertexB : element.vertexA;

                if (!mesh.is_valid(neighbor) || neighbor == vertex) {
                    return false;
                }

                append_unique(neighbors, neighbor);
            }

            if (incidentEdges.size() == 2u
                && neighbors.size() != 2u) {
                return false;
            }

            for (const geometry::EdgeHandle edge : incidentEdges) {
                if (!editor.remove_edge_if_loose(edge)) {
                    return false;
                }
            }

            if (neighbors.size() == 2u) {
                (void)editor.find_or_create_edge(neighbors[0], neighbors[1]);
            }

            return editor.remove_vertex_if_loose(vertex);
        }

    } // namespace

    DissolveMeshElementsOp DissolveMeshElementsOp::vertices(
        std::vector<geometry::VertexHandle> vertices)
    {
        DissolveMeshElementsOp operation{ DissolveMeshElementMode::Vertices };
        operation.vertices_ = std::move(vertices);
        return operation;
    }

    DissolveMeshElementsOp DissolveMeshElementsOp::edges(
        std::vector<geometry::EdgeHandle> edges)
    {
        DissolveMeshElementsOp operation{ DissolveMeshElementMode::Edges };
        operation.edges_ = std::move(edges);
        return operation;
    }

    DissolveMeshElementsOp DissolveMeshElementsOp::faces(
        std::vector<geometry::FaceHandle> faces)
    {
        DissolveMeshElementsOp operation{ DissolveMeshElementMode::Faces };
        operation.faces_ = std::move(faces);
        return operation;
    }

    std::string_view DissolveMeshElementsOp::name() const
    {
        return "DissolveMeshElementsOp";
    }

    DissolveMeshElementMode DissolveMeshElementsOp::mode() const
    {
        return mode_;
    }

    const std::vector<geometry::VertexHandle>&
        DissolveMeshElementsOp::vertices() const
    {
        return vertices_;
    }

    const std::vector<geometry::EdgeHandle>&
        DissolveMeshElementsOp::edges() const
    {
        return edges_;
    }

    const std::vector<geometry::FaceHandle>&
        DissolveMeshElementsOp::faces() const
    {
        return faces_;
    }

    DissolveMeshElementsOp::DissolveMeshElementsOp(
        DissolveMeshElementMode mode)
        : mode_(mode)
    {
    }

    OperationResult DissolveMeshElementsOp::execute_impl(
        OperationContext& context)
    {
        switch (mode_) {
        case DissolveMeshElementMode::Vertices:
            return execute_vertices(context);

        case DissolveMeshElementMode::Edges:
            return execute_edges(context);

        case DissolveMeshElementMode::Faces:
            return execute_faces(context);
        }

        return OperationResult::no_change(
            "Dissolve operation has no valid target mode.");
    }

    OperationResult DissolveMeshElementsOp::execute_vertices(
        OperationContext& context) const
    {
        geometry::LEM& mesh = context.editable_mesh();

        if (!validate_targets(mesh, vertices_)) {
            return invalid_targets_result("vertex");
        }

        const std::vector<geometry::VertexHandle> targets =
            unique_targets(vertices_);
        geometry::LEM candidate = mesh;
        geometry::LEMEditor editor(candidate);
        std::size_t dissolvedCount = 0u;

        for (const geometry::VertexHandle vertex : targets) {
            if (!candidate.is_valid(vertex)
                || !dissolve_one_vertex(candidate, editor, vertex)) {
                return OperationResult::fail(
                    kernel::ErrorCode::InvalidState,
                    "Dissolve Vertex is not valid for the selected topology.");
            }

            ++dissolvedCount;
        }

        if (dissolvedCount == 0u) {
            return OperationResult::no_change(
                "Dissolve operation did not remove any vertex.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        geometry::LEMDiff diff = editor.take_diff();
        mesh = std::move(candidate);
        return OperationResult::success(std::move(diff));
    }

    OperationResult DissolveMeshElementsOp::execute_edges(
        OperationContext& context) const
    {
        geometry::LEM& mesh = context.editable_mesh();

        if (!validate_targets(mesh, edges_)) {
            return invalid_targets_result("edge");
        }

        const std::vector<geometry::EdgeHandle> targets =
            unique_targets(edges_);
        geometry::LEM candidate = mesh;
        geometry::LEMEditor editor(candidate);
        std::size_t dissolvedCount = 0u;

        for (const geometry::EdgeHandle edge : targets) {
            if (!candidate.is_valid(edge)
                || !dissolve_one_edge(candidate, editor, edge)) {
                return OperationResult::fail(
                    kernel::ErrorCode::InvalidState,
                    "Dissolve Edge is not valid for the selected topology.");
            }

            ++dissolvedCount;
        }

        if (dissolvedCount == 0u) {
            return OperationResult::no_change(
                "Dissolve operation did not remove any edge.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        geometry::LEMDiff diff = editor.take_diff();
        mesh = std::move(candidate);
        return OperationResult::success(std::move(diff));
    }

    OperationResult DissolveMeshElementsOp::execute_faces(
        OperationContext& context) const
    {
        const geometry::LEM& mesh = context.editable_mesh();

        if (!validate_targets(mesh, faces_)) {
            return invalid_targets_result("face");
        }

        return OperationResult::fail(
            kernel::ErrorCode::InvalidState,
            "Dissolve Face is not exposed because the current primitive "
            "matches Delete Face semantics instead of preserving a surface.");
    }

} // namespace locus::kernel::modeling
