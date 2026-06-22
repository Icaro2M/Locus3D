/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/topology/TopologyRelink.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Loop.h"
#include "kernel/geometry/mesh/elements/Vertex.h"

namespace locus::kernel::geometry::editing::topology {

    bool remove_loop_from_radial(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle)
    {
        if (!mesh.is_valid(loopHandle)) {
            return false;
        }

        Loop& loop = mesh.loop(loopHandle);

        if (!mesh.is_valid(loop.edge)) {
            loop.radialNext = {};
            loop.radialPrevious = {};
            diff.record(LEMChangeType::LoopModified, loopHandle);
            return true;
        }

        const EdgeHandle edgeHandle = loop.edge;
        Edge& edge = mesh.edge(edgeHandle);

        const LoopHandle next = loop.radialNext;
        const LoopHandle previous = loop.radialPrevious;

        if (next == loopHandle && previous == loopHandle) {
            if (edge.loop == loopHandle) {
                edge.loop = {};
                diff.record(LEMChangeType::EdgeModified, edgeHandle);
            }

            loop.radialNext = {};
            loop.radialPrevious = {};
            diff.record(LEMChangeType::LoopModified, loopHandle);
            return true;
        }

        if (mesh.is_valid(next)) {
            mesh.loop(next).radialPrevious = previous;
            diff.record(LEMChangeType::LoopModified, next);
        }

        if (mesh.is_valid(previous)) {
            mesh.loop(previous).radialNext = next;
            diff.record(LEMChangeType::LoopModified, previous);
        }

        if (edge.loop == loopHandle) {
            edge.loop = mesh.is_valid(next) && next != loopHandle ? next : LoopHandle{};
            diff.record(LEMChangeType::EdgeModified, edgeHandle);
        }

        loop.radialNext = {};
        loop.radialPrevious = {};
        diff.record(LEMChangeType::LoopModified, loopHandle);

        return true;
    }

    bool insert_loop_into_radial(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle, EdgeHandle edgeHandle)
    {
        if (!mesh.is_valid(loopHandle) || !mesh.is_valid(edgeHandle)) {
            return false;
        }

        Loop& loop = mesh.loop(loopHandle);
        Edge& edge = mesh.edge(edgeHandle);

        loop.edge = edgeHandle;

        if (!mesh.is_valid(edge.loop)) {
            edge.loop = loopHandle;
            loop.radialNext = loopHandle;
            loop.radialPrevious = loopHandle;

            diff.record(LEMChangeType::LoopModified, loopHandle);
            diff.record(LEMChangeType::EdgeModified, edgeHandle);
            return true;
        }

        const LoopHandle entry = edge.loop;

        if (!mesh.is_valid(entry)) {
            edge.loop = loopHandle;
            loop.radialNext = loopHandle;
            loop.radialPrevious = loopHandle;

            diff.record(LEMChangeType::LoopModified, loopHandle);
            diff.record(LEMChangeType::EdgeModified, edgeHandle);
            return true;
        }

        const LoopHandle previous = mesh.loop(entry).radialPrevious;

        if (!mesh.is_valid(previous)) {
            return false;
        }

        loop.radialNext = entry;
        loop.radialPrevious = previous;
        mesh.loop(previous).radialNext = loopHandle;
        mesh.loop(entry).radialPrevious = loopHandle;

        diff.record(LEMChangeType::LoopModified, loopHandle);
        diff.record(LEMChangeType::LoopModified, previous);
        diff.record(LEMChangeType::LoopModified, entry);
        diff.record(LEMChangeType::EdgeModified, edgeHandle);

        return true;
    }

    void refresh_edge_entry_loop(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle)
    {
        if (!mesh.is_valid(edgeHandle)) {
            return;
        }

        Edge& edge = mesh.edge(edgeHandle);

        if (mesh.is_valid(edge.loop) && mesh.loop(edge.loop).edge == edgeHandle) {
            return;
        }

        edge.loop = {};

        for (std::size_t index = 0; index < mesh.loop_count(); ++index) {
            LoopHandle loopHandle(index);

            if (!mesh.is_valid(loopHandle)) {
                continue;
            }

            const Loop& loop = mesh.loop(loopHandle);

            if (loop.edge == edgeHandle) {
                edge.loop = loopHandle;
                diff.record(LEMChangeType::EdgeModified, edgeHandle);
                return;
            }
        }

        diff.record(LEMChangeType::EdgeModified, edgeHandle);
    }

    void refresh_vertex_incident_edge(LEM& mesh, LEMDiff& diff, VertexHandle vertexHandle)
    {
        if (!mesh.is_valid(vertexHandle)) {
            return;
        }

        Vertex& vertex = mesh.vertex(vertexHandle);

        if (mesh.is_valid(vertex.edge)) {
            const Edge& edge = mesh.edge(vertex.edge);

            if (edge.vertexA == vertexHandle || edge.vertexB == vertexHandle) {
                return;
            }
        }

        vertex.edge = {};

        for (std::size_t index = 0; index < mesh.edge_count(); ++index) {
            EdgeHandle edgeHandle(index);

            if (!mesh.is_valid(edgeHandle)) {
                continue;
            }

            const Edge& edge = mesh.edge(edgeHandle);

            if (edge.vertexA == vertexHandle || edge.vertexB == vertexHandle) {
                vertex.edge = edgeHandle;
                diff.record(LEMChangeType::VertexModified, vertexHandle);
                return;
            }
        }

        diff.record(LEMChangeType::VertexModified, vertexHandle);
    }

    bool update_loop_vertex(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle, VertexHandle vertexHandle)
    {
        if (!mesh.is_valid(loopHandle) || !mesh.is_valid(vertexHandle)) {
            return false;
        }

        Loop& loop = mesh.loop(loopHandle);

        if (loop.vertex == vertexHandle) {
            return true;
        }

        const VertexHandle oldVertex = loop.vertex;
        loop.vertex = vertexHandle;

        if (mesh.is_valid(oldVertex)) {
            refresh_vertex_incident_edge(mesh, diff, oldVertex);
        }

        refresh_vertex_incident_edge(mesh, diff, vertexHandle);

        diff.record(LEMChangeType::LoopModified, loopHandle);
        diff.record(LEMChangeType::VertexModified, vertexHandle);

        return true;
    }

    bool update_loop_edge(LEM& mesh, LEMDiff& diff, LoopHandle loopHandle, EdgeHandle edgeHandle)
    {
        if (!mesh.is_valid(loopHandle) || !mesh.is_valid(edgeHandle)) {
            return false;
        }

        Loop& loop = mesh.loop(loopHandle);

        if (loop.edge == edgeHandle) {
            return true;
        }

        const EdgeHandle oldEdge = loop.edge;

        if (mesh.is_valid(oldEdge)) {
            remove_loop_from_radial(mesh, diff, loopHandle);
            refresh_edge_entry_loop(mesh, diff, oldEdge);
        }

        if (!insert_loop_into_radial(mesh, diff, loopHandle, edgeHandle)) {
            return false;
        }

        refresh_edge_entry_loop(mesh, diff, edgeHandle);
        diff.record(LEMChangeType::LoopModified, loopHandle);

        return true;
    }

    bool replace_vertex_in_face(
        LEM& mesh,
        LEMDiff& diff,
        FaceHandle faceHandle,
        VertexHandle oldVertex,
        VertexHandle newVertex)
    {
        if (!mesh.is_valid(faceHandle) || !mesh.is_valid(oldVertex) || !mesh.is_valid(newVertex)) {
            return false;
        }

        bool changed = false;

        for (LoopHandle loopHandle : mesh.face_loops(faceHandle)) {
            if (!mesh.is_valid(loopHandle)) {
                continue;
            }

            Loop& loop = mesh.loop(loopHandle);

            if (loop.vertex != oldVertex) {
                continue;
            }

            loop.vertex = newVertex;
            diff.record(LEMChangeType::LoopModified, loopHandle);
            changed = true;
        }

        if (changed) {
            refresh_vertex_incident_edge(mesh, diff, oldVertex);
            refresh_vertex_incident_edge(mesh, diff, newVertex);
            diff.record(LEMChangeType::FaceModified, faceHandle);
        }

        return changed;
    }

}