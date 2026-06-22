/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/TopologyEditor.h"

#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>

namespace locus::kernel::geometry {

    TopologyEditor::TopologyEditor(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff) {
    }

    LEM& TopologyEditor::mesh() {
        return mesh_;
    }

    const LEM& TopologyEditor::mesh() const {
        return mesh_;
    }

    VertexHandle TopologyEditor::add_vertex(const glm::vec3& position) {
        VertexHandle handle = mesh_.add_vertex(position);

        if (mesh_.is_valid(handle)) {
            diff_.record(LEMChangeType::VertexAdded, handle);
        }

        return handle;
    }

    EdgeHandle TopologyEditor::find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB) {
        const std::size_t edgeCount = mesh_.edge_count();
        EdgeHandle handle = mesh_.find_or_create_edge(vertexA, vertexB);

        if (mesh_.is_valid(handle) && mesh_.edge_count() > edgeCount) {
            diff_.record(LEMChangeType::EdgeAdded, handle);

            if (mesh_.is_valid(vertexA)) {
                diff_.record(LEMChangeType::VertexModified, vertexA);
            }

            if (mesh_.is_valid(vertexB)) {
                diff_.record(LEMChangeType::VertexModified, vertexB);
            }
        }

        return handle;
    }

    FaceHandle TopologyEditor::add_face(const std::vector<VertexHandle>& vertices) {
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

    bool TopologyEditor::remove_face(FaceHandle faceHandle) {
        if (!mesh_.is_valid(faceHandle)) {
            return false;
        }

        const std::vector<LoopHandle> loops = mesh_.face_loops(faceHandle);

        if (loops.empty()) {
            mesh_.face(faceHandle).deleted = true;
            diff_.record(LEMChangeType::FaceModified, faceHandle);
            return true;
        }

        std::vector<EdgeHandle> affectedEdges;
        std::vector<VertexHandle> affectedVertices;

        affectedEdges.reserve(loops.size());
        affectedVertices.reserve(loops.size());

        for (LoopHandle loopHandle : loops) {
            if (!mesh_.is_valid(loopHandle)) {
                continue;
            }

            const Loop& loop = mesh_.loop(loopHandle);

            if (mesh_.is_valid(loop.edge) && std::find(affectedEdges.begin(), affectedEdges.end(), loop.edge) == affectedEdges.end()) {
                affectedEdges.push_back(loop.edge);
            }

            if (mesh_.is_valid(loop.vertex) && std::find(affectedVertices.begin(), affectedVertices.end(), loop.vertex) == affectedVertices.end()) {
                affectedVertices.push_back(loop.vertex);
            }
        }

        for (LoopHandle loopHandle : loops) {
            if (!mesh_.is_valid(loopHandle)) {
                continue;
            }

            remove_loop_from_radial(loopHandle);

            Loop& loop = mesh_.loop(loopHandle);
            loop.next = {};
            loop.previous = {};
            loop.radialNext = {};
            loop.radialPrevious = {};
            loop.deleted = true;

            diff_.record(LEMChangeType::LoopModified, loopHandle);
        }

        Face& face = mesh_.face(faceHandle);
        face.loop = {};
        face.deleted = true;

        diff_.record(LEMChangeType::FaceModified, faceHandle);

        for (EdgeHandle edgeHandle : affectedEdges) {
            refresh_edge_entry_loop(edgeHandle);
            diff_.record(LEMChangeType::EdgeModified, edgeHandle);
        }

        for (VertexHandle vertexHandle : affectedVertices) {
            refresh_vertex_incident_edge(vertexHandle);
            diff_.record(LEMChangeType::VertexModified, vertexHandle);
        }

        return true;
    }

    bool TopologyEditor::remove_edge_if_loose(EdgeHandle edgeHandle) {
        if (!mesh_.is_valid(edgeHandle)) {
            return false;
        }

        if (!TopologyTraversal::edge_loops(mesh_, edgeHandle).empty()) {
            return false;
        }

        Edge& edge = mesh_.edge(edgeHandle);

        const VertexHandle vertexA = edge.vertexA;
        const VertexHandle vertexB = edge.vertexB;

        edge.vertexA = {};
        edge.vertexB = {};
        edge.loop = {};
        edge.deleted = true;

        diff_.record(LEMChangeType::EdgeModified, edgeHandle);

        refresh_vertex_incident_edge(vertexA);
        refresh_vertex_incident_edge(vertexB);

        if (mesh_.is_valid(vertexA)) {
            diff_.record(LEMChangeType::VertexModified, vertexA);
        }

        if (mesh_.is_valid(vertexB)) {
            diff_.record(LEMChangeType::VertexModified, vertexB);
        }

        return true;
    }

    bool TopologyEditor::remove_vertex_if_loose(VertexHandle vertexHandle) {
        if (!mesh_.is_valid(vertexHandle)) {
            return false;
        }

        if (!TopologyTraversal::vertex_edges(mesh_, vertexHandle).empty()) {
            return false;
        }

        if (!TopologyTraversal::vertex_loops(mesh_, vertexHandle).empty()) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(vertexHandle);
        vertex.edge = {};
        vertex.deleted = true;

        diff_.record(LEMChangeType::VertexModified, vertexHandle);

        return true;
    }

    bool TopologyEditor::flip_face(FaceHandle faceHandle) {
        if (!mesh_.is_valid(faceHandle)) {
            return false;
        }

        const std::vector<LoopHandle> loops = mesh_.face_loops(faceHandle);

        if (loops.size() < 3) {
            return false;
        }

        for (LoopHandle loopHandle : loops) {
            if (!mesh_.is_valid(loopHandle)) {
                return false;
            }
        }

        for (LoopHandle loopHandle : loops) {
            remove_loop_from_radial(loopHandle);
        }

        for (LoopHandle loopHandle : loops) {
            Loop& loop = mesh_.loop(loopHandle);
            std::swap(loop.next, loop.previous);
        }

        for (LoopHandle loopHandle : loops) {
            Loop& loop = mesh_.loop(loopHandle);

            if (!mesh_.is_valid(loop.next)) {
                return false;
            }

            const VertexHandle currentVertex = loop.vertex;
            const VertexHandle nextVertex = mesh_.loop(loop.next).vertex;
            const EdgeHandle newEdge = mesh_.find_edge(currentVertex, nextVertex);

            if (!mesh_.is_valid(newEdge)) {
                return false;
            }

            if (!insert_loop_into_radial(loopHandle, newEdge)) {
                return false;
            }

            diff_.record(LEMChangeType::LoopModified, loopHandle);
        }

        Face& face = mesh_.face(faceHandle);
        face.normal = NormalBuilder::face_normal(mesh_, faceHandle);

        diff_.record(LEMChangeType::FaceModified, faceHandle);
        diff_.record(LEMChangeType::NormalsChanged, faceHandle);

        return true;
    }

    std::size_t TopologyEditor::flip_all_faces() {
        std::size_t count = 0;

        for (FaceHandle faceHandle : TopologyTraversal::faces(mesh_)) {
            if (flip_face(faceHandle)) {
                ++count;
            }
        }

        return count;
    }

    void TopologyEditor::rebuild_face_normals() {
        NormalBuilder::rebuild_face_normals(mesh_);

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            diff_.record(LEMChangeType::NormalsChanged, handle);
        }
    }

    void TopologyEditor::clear() {
        mesh_.clear();
        diff_.record(LEMChangeType::MeshCleared, LEMElementType::Vertex, Id{});
    }

    bool TopologyEditor::remove_loop_from_radial(LoopHandle loopHandle) {
        if (!mesh_.is_valid(loopHandle)) {
            return false;
        }

        Loop& loop = mesh_.loop(loopHandle);

        if (!mesh_.is_valid(loop.edge)) {
            loop.radialNext = {};
            loop.radialPrevious = {};
            return true;
        }

        Edge& edge = mesh_.edge(loop.edge);

        const LoopHandle next = loop.radialNext;
        const LoopHandle previous = loop.radialPrevious;

        if (next == loopHandle && previous == loopHandle) {
            if (edge.loop == loopHandle) {
                edge.loop = {};
            }

            loop.radialNext = {};
            loop.radialPrevious = {};
            return true;
        }

        if (mesh_.is_valid(next)) {
            mesh_.loop(next).radialPrevious = previous;
            diff_.record(LEMChangeType::LoopModified, next);
        }

        if (mesh_.is_valid(previous)) {
            mesh_.loop(previous).radialNext = next;
            diff_.record(LEMChangeType::LoopModified, previous);
        }

        if (edge.loop == loopHandle) {
            edge.loop = (mesh_.is_valid(next) && next != loopHandle) ? next : LoopHandle{};
        }

        loop.radialNext = {};
        loop.radialPrevious = {};

        return true;
    }

    bool TopologyEditor::insert_loop_into_radial(LoopHandle loopHandle, EdgeHandle edgeHandle) {
        if (!mesh_.is_valid(loopHandle) || !mesh_.is_valid(edgeHandle)) {
            return false;
        }

        Loop& loop = mesh_.loop(loopHandle);
        Edge& edge = mesh_.edge(edgeHandle);

        loop.edge = edgeHandle;

        if (!mesh_.is_valid(edge.loop)) {
            edge.loop = loopHandle;
            loop.radialNext = loopHandle;
            loop.radialPrevious = loopHandle;

            diff_.record(LEMChangeType::EdgeModified, edgeHandle);
            return true;
        }

        const LoopHandle entry = edge.loop;

        if (!mesh_.is_valid(entry)) {
            edge.loop = loopHandle;
            loop.radialNext = loopHandle;
            loop.radialPrevious = loopHandle;

            diff_.record(LEMChangeType::EdgeModified, edgeHandle);
            return true;
        }

        const LoopHandle previous = mesh_.loop(entry).radialPrevious;

        if (!mesh_.is_valid(previous)) {
            return false;
        }

        loop.radialNext = entry;
        loop.radialPrevious = previous;
        mesh_.loop(previous).radialNext = loopHandle;
        mesh_.loop(entry).radialPrevious = loopHandle;

        diff_.record(LEMChangeType::LoopModified, previous);
        diff_.record(LEMChangeType::LoopModified, entry);
        diff_.record(LEMChangeType::EdgeModified, edgeHandle);

        return true;
    }

    void TopologyEditor::refresh_edge_entry_loop(EdgeHandle edgeHandle) {
        if (!mesh_.is_valid(edgeHandle)) {
            return;
        }

        Edge& edge = mesh_.edge(edgeHandle);

        if (mesh_.is_valid(edge.loop) && mesh_.loop(edge.loop).edge == edgeHandle) {
            return;
        }

        edge.loop = {};

        for (std::size_t index = 0; index < mesh_.loop_count(); ++index) {
            LoopHandle loopHandle(static_cast<IdValue>(index));

            if (!mesh_.is_valid(loopHandle)) {
                continue;
            }

            const Loop& loop = mesh_.loop(loopHandle);

            if (loop.edge == edgeHandle) {
                edge.loop = loopHandle;
                return;
            }
        }
    }

    void TopologyEditor::refresh_vertex_incident_edge(VertexHandle vertexHandle) {
        if (!mesh_.is_valid(vertexHandle)) {
            return;
        }

        Vertex& vertex = mesh_.vertex(vertexHandle);

        if (mesh_.is_valid(vertex.edge)) {
            const Edge& edge = mesh_.edge(vertex.edge);

            if (edge.vertexA == vertexHandle || edge.vertexB == vertexHandle) {
                return;
            }
        }

        vertex.edge = {};

        for (std::size_t index = 0; index < mesh_.edge_count(); ++index) {
            EdgeHandle edgeHandle(static_cast<IdValue>(index));

            if (!mesh_.is_valid(edgeHandle)) {
                continue;
            }

            const Edge& edge = mesh_.edge(edgeHandle);

            if (edge.vertexA == vertexHandle || edge.vertexB == vertexHandle) {
                vertex.edge = edgeHandle;
                return;
            }
        }
    }

}