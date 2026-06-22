/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/topology/TopologyFlip.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/editing/topology/TopologyCreation.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRemoval.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRelink.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Face.h"
#include "kernel/geometry/mesh/elements/Loop.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace locus::kernel::geometry::editing::topology {

    namespace {

        /**
         * @brief Finds the vertex opposite to an edge inside a triangular face.
         *
         * @param vertices Ordered vertices of a triangular face.
         * @param vertexA First edge endpoint.
         * @param vertexB Second edge endpoint.
         * @return Opposite vertex when the face is triangular and contains the edge.
         */
        std::optional<VertexHandle> opposite_triangle_vertex(
            const std::vector<VertexHandle>& vertices,
            VertexHandle vertexA,
            VertexHandle vertexB)
        {
            if (vertices.size() != 3) {
                return std::nullopt;
            }

            bool containsA = false;
            bool containsB = false;
            std::optional<VertexHandle> opposite = std::nullopt;

            for (VertexHandle vertexHandle : vertices) {
                if (vertexHandle == vertexA) {
                    containsA = true;
                    continue;
                }

                if (vertexHandle == vertexB) {
                    containsB = true;
                    continue;
                }

                opposite = vertexHandle;
            }

            if (!containsA || !containsB || !opposite.has_value()) {
                return std::nullopt;
            }

            return opposite;
        }

    }

    bool flip_face(LEM& mesh, LEMDiff& diff, FaceHandle faceHandle)
    {
        if (!mesh.is_valid(faceHandle)) {
            return false;
        }

        const std::vector<LoopHandle> loops = mesh.face_loops(faceHandle);

        if (loops.size() < 3) {
            return false;
        }

        for (LoopHandle loopHandle : loops) {
            if (!mesh.is_valid(loopHandle)) {
                return false;
            }
        }

        for (LoopHandle loopHandle : loops) {
            remove_loop_from_radial(mesh, diff, loopHandle);
        }

        for (LoopHandle loopHandle : loops) {
            Loop& loop = mesh.loop(loopHandle);
            std::swap(loop.next, loop.previous);
            diff.record(LEMChangeType::LoopModified, loopHandle);
        }

        for (LoopHandle loopHandle : loops) {
            Loop& loop = mesh.loop(loopHandle);

            if (!mesh.is_valid(loop.next)) {
                return false;
            }

            const VertexHandle currentVertex = loop.vertex;
            const VertexHandle nextVertex = mesh.loop(loop.next).vertex;
            const EdgeHandle newEdge = mesh.find_edge(currentVertex, nextVertex);

            if (!mesh.is_valid(newEdge)) {
                return false;
            }

            if (!insert_loop_into_radial(mesh, diff, loopHandle, newEdge)) {
                return false;
            }

            diff.record(LEMChangeType::LoopModified, loopHandle);
        }

        Face& face = mesh.face(faceHandle);
        face.normal = NormalBuilder::face_normal(mesh, faceHandle);

        diff.record(LEMChangeType::FaceModified, faceHandle);
        diff.record(LEMChangeType::NormalsChanged, faceHandle);

        return true;
    }

    std::size_t flip_all_faces(LEM& mesh, LEMDiff& diff)
    {
        std::size_t flippedCount = 0;

        for (FaceHandle faceHandle : TopologyTraversal::faces(mesh)) {
            if (flip_face(mesh, diff, faceHandle)) {
                ++flippedCount;
            }
        }

        return flippedCount;
    }

    bool flip_edge(LEM& mesh, LEMDiff& diff, EdgeHandle edgeHandle)
    {
        if (!mesh.is_valid(edgeHandle)) {
            return false;
        }

        const Edge edge = mesh.edge(edgeHandle);

        if (!mesh.is_valid(edge.vertexA) || !mesh.is_valid(edge.vertexB)) {
            return false;
        }

        const std::vector<FaceHandle> faces = TopologyTraversal::edge_faces(mesh, edgeHandle);

        if (faces.size() != 2) {
            return false;
        }

        const std::vector<VertexHandle> firstVertices = TopologyTraversal::face_vertices(mesh, faces[0]);
        const std::vector<VertexHandle> secondVertices = TopologyTraversal::face_vertices(mesh, faces[1]);

        const std::optional<VertexHandle> firstOpposite =
            opposite_triangle_vertex(firstVertices, edge.vertexA, edge.vertexB);

        const std::optional<VertexHandle> secondOpposite =
            opposite_triangle_vertex(secondVertices, edge.vertexA, edge.vertexB);

        if (!firstOpposite.has_value() || !secondOpposite.has_value()) {
            return false;
        }

        const VertexHandle firstOppositeVertex = firstOpposite.value();
        const VertexHandle secondOppositeVertex = secondOpposite.value();

        if (firstOppositeVertex == secondOppositeVertex) {
            return false;
        }

        remove_face(mesh, diff, faces[0]);
        remove_face(mesh, diff, faces[1]);
        kill_edge_only(mesh, diff, edgeHandle);

        add_face(mesh, diff, std::vector<VertexHandle>{
            firstOppositeVertex,
                secondOppositeVertex,
                edge.vertexB
        });

        add_face(mesh, diff, std::vector<VertexHandle>{
            secondOppositeVertex,
                firstOppositeVertex,
                edge.vertexA
        });

        return true;
    }

}