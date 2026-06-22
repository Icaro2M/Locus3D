/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/topology/TopologyRemoval.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRelink.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Face.h"
#include "kernel/geometry/mesh/elements/Loop.h"
#include "kernel/geometry/mesh/elements/Vertex.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <vector>

namespace locus::kernel::geometry::editing::topology {

    namespace {

        template <typename HandleT>
        bool contains_handle(const std::vector<HandleT>& handles, HandleT handle)
        {
            return std::find(handles.begin(), handles.end(), handle) != handles.end();
        }

    }

    bool remove_face(LEM& mesh, LEMDiff& diff, FaceHandle faceHandle)
    {
        if (!mesh.is_valid(faceHandle)) {
            return false;
        }

        const std::vector<LoopHandle> loops = mesh.face_loops(faceHandle);

        if (loops.empty()) {
            Face& face = mesh.face(faceHandle);
            face.loop = {};
            face.deleted = true;
            diff.record(LEMChangeType::FaceModified, faceHandle);
            return true;
        }

        std::vector<EdgeHandle> affectedEdges;
        std::vector<VertexHandle> affectedVertices;

        affectedEdges.reserve(loops.size());
        affectedVertices.reserve(loops.size());

        for (LoopHandle loopHandle : loops) {
            if (!mesh.is_valid(loopHandle)) {
                continue;
            }

            const Loop& loop = mesh.loop(loopHandle);

            if (mesh.is_valid(loop.edge) && !contains_handle(affectedEdges, loop.edge)) {
                affectedEdges.push_back(loop.edge);
            }

            if (mesh.is_valid(loop.vertex) && !contains_handle(affectedVertices, loop.vertex)) {
                affectedVertices.push_back(loop.vertex);
            }
        }

        for (LoopHandle loopHandle : loops) {
            kill_loop(mesh, diff, loopHandle);
        }

        Face& face = mesh.face(faceHandle);
        face.loop = {};
        face.deleted = true;
        diff.record(LEMChangeType::FaceModified, faceHandle);

        for (EdgeHandle edgeHandle : affectedEdges) {
            refresh_edge_entry_loop(mesh, diff, edgeHandle);
        }

        for (VertexHandle vertexHandle : affectedVertices) {
            refresh_vertex_incident_edge(mesh, diff, vertexHandle);
        }

        return true;
    }

    bool remove_edge_if_loose(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle)
    {
        if (!mesh.is_valid(edgeHandle)) {
            return false;
        }

        if (!TopologyTraversal::edge_loops(mesh, edgeHandle).empty()) {
            return false;
        }

        return kill_edge_only(mesh, diff, edgeHandle);
    }

    bool remove_vertex_if_loose(LEM& mesh, LEMDiff& diff, VertexHandle vertexHandle)
    {
        if (!mesh.is_valid(vertexHandle)) {
            return false;
        }

        if (!TopologyTraversal::vertex_edges(mesh, vertexHandle).empty()) {
            return false;
        }

        if (!TopologyTraversal::vertex_loops(mesh, vertexHandle).empty()) {
            return false;
        }

        Vertex& vertex = mesh.vertex(vertexHandle);
        vertex.edge = {};
        vertex.deleted = true;

        diff.record(LEMChangeType::VertexModified, vertexHandle);

        return true;
    }

    bool kill_face_only(LEM& mesh, LEMDiff& diff, FaceHandle faceHandle)
    {
        if (!mesh.is_valid(faceHandle)) {
            return false;
        }

        Face& face = mesh.face(faceHandle);
        face.loop = {};
        face.deleted = true;

        diff.record(LEMChangeType::FaceModified, faceHandle);

        return true;
    }

    bool kill_edge_only(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle)
    {
        if (!mesh.is_valid(edgeHandle)) {
            return false;
        }

        Edge& edge = mesh.edge(edgeHandle);

        const VertexHandle vertexA = edge.vertexA;
        const VertexHandle vertexB = edge.vertexB;

        for (LoopHandle loopHandle : TopologyTraversal::edge_loops(mesh, edgeHandle)) {
            if (mesh.is_valid(loopHandle)) {
                mesh.loop(loopHandle).edge = {};
                mesh.loop(loopHandle).radialNext = {};
                mesh.loop(loopHandle).radialPrevious = {};
                diff.record(LEMChangeType::LoopModified, loopHandle);
            }
        }

        edge.vertexA = {};
        edge.vertexB = {};
        edge.loop = {};
        edge.deleted = true;

        diff.record(LEMChangeType::EdgeModified, edgeHandle);

        refresh_vertex_incident_edge(mesh, diff, vertexA);
        refresh_vertex_incident_edge(mesh, diff, vertexB);

        return true;
    }

    bool kill_loop(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle)
    {
        if (!mesh.is_valid(loopHandle)) {
            return false;
        }

        Loop& loop = mesh.loop(loopHandle);

        const FaceHandle faceHandle = loop.face;
        const EdgeHandle edgeHandle = loop.edge;
        const VertexHandle vertexHandle = loop.vertex;
        const LoopHandle next = loop.next;
        const LoopHandle previous = loop.previous;

        remove_loop_from_radial(mesh, diff, loopHandle);

        if (mesh.is_valid(previous) && mesh.loop(previous).next == loopHandle) {
            mesh.loop(previous).next = next;
            diff.record(LEMChangeType::LoopModified, previous);
        }

        if (mesh.is_valid(next) && mesh.loop(next).previous == loopHandle) {
            mesh.loop(next).previous = previous;
            diff.record(LEMChangeType::LoopModified, next);
        }

        if (mesh.is_valid(faceHandle)) {
            Face& face = mesh.face(faceHandle);

            if (face.loop == loopHandle) {
                face.loop = mesh.is_valid(next) && next != loopHandle ? next : LoopHandle{};
                diff.record(LEMChangeType::FaceModified, faceHandle);
            }
        }

        loop.vertex = {};
        loop.edge = {};
        loop.face = {};
        loop.next = {};
        loop.previous = {};
        loop.radialNext = {};
        loop.radialPrevious = {};
        loop.deleted = true;

        diff.record(LEMChangeType::LoopModified, loopHandle);

        refresh_edge_entry_loop(mesh, diff, edgeHandle);
        refresh_vertex_incident_edge(mesh, diff, vertexHandle);

        return true;
    }

}