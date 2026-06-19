/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Convenience queries for mesh adjacency relationships.
     */
    class AdjacencyQuery {
    public:
        /**
         * @brief Returns edges incident to a vertex.
         *
         * @param mesh Mesh to query.
         * @param vertex Vertex whose incident edges are returned.
         * @return Edge handles connected to the vertex.
         */
        [[nodiscard]] static std::vector<EdgeHandle> vertex_edges(const LEM& mesh, VertexHandle vertex)
        {
            return TopologyTraversal::vertex_edges(mesh, vertex);
        }

        /**
         * @brief Returns loops that reference a vertex.
         *
         * @param mesh Mesh to query.
         * @param vertex Vertex whose loops are returned.
         * @return Loop handles using the vertex.
         */
        [[nodiscard]] static std::vector<LoopHandle> vertex_loops(const LEM& mesh, VertexHandle vertex)
        {
            return TopologyTraversal::vertex_loops(mesh, vertex);
        }

        /**
         * @brief Returns faces adjacent to a vertex.
         *
         * @param mesh Mesh to query.
         * @param vertex Vertex whose adjacent faces are returned.
         * @return Face handles using the vertex.
         */
        [[nodiscard]] static std::vector<FaceHandle> vertex_faces(const LEM& mesh, VertexHandle vertex)
        {
            return TopologyTraversal::vertex_faces(mesh, vertex);
        }

        /**
         * @brief Returns vertices connected to a vertex by an edge.
         *
         * @param mesh Mesh to query.
         * @param vertex Vertex whose neighbors are returned.
         * @return Adjacent vertex handles.
         */
        [[nodiscard]] static std::vector<VertexHandle> adjacent_vertices(const LEM& mesh, VertexHandle vertex)
        {
            return TopologyTraversal::adjacent_vertices(mesh, vertex);
        }

        /**
         * @brief Returns valid endpoint vertices for an edge.
         *
         * @param mesh Mesh to query.
         * @param edge Edge whose endpoints are returned.
         * @return One or two valid endpoint handles.
         */
        [[nodiscard]] static std::vector<VertexHandle> edge_vertices(const LEM& mesh, EdgeHandle edge)
        {
            std::vector<VertexHandle> result;

            if (!mesh.is_valid(edge)) {
                return result;
            }

            const Edge& edgeElement = mesh.edge(edge);

            if (mesh.is_valid(edgeElement.vertexA)) {
                result.push_back(edgeElement.vertexA);
            }

            if (mesh.is_valid(edgeElement.vertexB) && edgeElement.vertexB != edgeElement.vertexA) {
                result.push_back(edgeElement.vertexB);
            }

            return result;
        }

        /**
         * @brief Returns loops in an edge radial cycle.
         *
         * @param mesh Mesh to query.
         * @param edge Edge whose loops are returned.
         * @return Loop handles using the edge.
         */
        [[nodiscard]] static std::vector<LoopHandle> edge_loops(const LEM& mesh, EdgeHandle edge)
        {
            return TopologyTraversal::edge_loops(mesh, edge);
        }

        /**
         * @brief Returns faces adjacent to an edge.
         *
         * @param mesh Mesh to query.
         * @param edge Edge whose adjacent faces are returned.
         * @return Face handles using the edge.
         */
        [[nodiscard]] static std::vector<FaceHandle> edge_faces(const LEM& mesh, EdgeHandle edge)
        {
            return TopologyTraversal::edge_faces(mesh, edge);
        }

        /**
         * @brief Returns loops around a face boundary.
         *
         * @param mesh Mesh to query.
         * @param face Face whose boundary loops are returned.
         * @return Ordered face loop handles.
         */
        [[nodiscard]] static std::vector<LoopHandle> face_loops(const LEM& mesh, FaceHandle face)
        {
            return TopologyTraversal::face_loops(mesh, face);
        }

        /**
         * @brief Returns vertices around a face boundary.
         *
         * @param mesh Mesh to query.
         * @param face Face whose vertices are returned.
         * @return Ordered face vertex handles.
         */
        [[nodiscard]] static std::vector<VertexHandle> face_vertices(const LEM& mesh, FaceHandle face)
        {
            return TopologyTraversal::face_vertices(mesh, face);
        }

        /**
         * @brief Returns unique edges around a face boundary.
         *
         * @param mesh Mesh to query.
         * @param face Face whose edges are returned.
         * @return Face edge handles.
         */
        [[nodiscard]] static std::vector<EdgeHandle> face_edges(const LEM& mesh, FaceHandle face)
        {
            return TopologyTraversal::face_edges(mesh, face);
        }

        /**
         * @brief Returns faces connected to a face through shared edges.
         *
         * @param mesh Mesh to query.
         * @param face Face whose edge-adjacent neighbors are returned.
         * @return Unique adjacent face handles.
         */
        [[nodiscard]] static std::vector<FaceHandle> adjacent_faces(const LEM& mesh, FaceHandle face)
        {
            std::vector<FaceHandle> result;

            if (!mesh.is_valid(face)) {
                return result;
            }

            for (EdgeHandle edge : TopologyTraversal::face_edges(mesh, face)) {
                for (FaceHandle adjacentFace : TopologyTraversal::edge_faces(mesh, edge)) {
                    if (adjacentFace != face && !contains(result, adjacentFace)) {
                        result.push_back(adjacentFace);
                    }
                }
            }

            return result;
        }

        /**
         * @brief Returns faces connected to a face through shared vertices.
         *
         * @param mesh Mesh to query.
         * @param face Face whose vertex-connected neighbors are returned.
         * @return Unique connected face handles.
         */
        [[nodiscard]] static std::vector<FaceHandle> connected_faces_by_vertex(const LEM& mesh, FaceHandle face)
        {
            std::vector<FaceHandle> result;

            if (!mesh.is_valid(face)) {
                return result;
            }

            for (VertexHandle vertex : TopologyTraversal::face_vertices(mesh, face)) {
                for (FaceHandle adjacentFace : TopologyTraversal::vertex_faces(mesh, vertex)) {
                    if (adjacentFace != face && !contains(result, adjacentFace)) {
                        result.push_back(adjacentFace);
                    }
                }
            }

            return result;
        }

        /**
         * @brief Checks whether two vertices share an edge.
         *
         * @param mesh Mesh to query.
         * @param a First vertex.
         * @param b Second vertex.
         * @return True when the vertices are connected by an edge.
         */
        [[nodiscard]] static bool are_vertices_adjacent(const LEM& mesh, VertexHandle a, VertexHandle b)
        {
            if (!mesh.is_valid(a) || !mesh.is_valid(b) || a == b) {
                return false;
            }

            for (VertexHandle adjacent : TopologyTraversal::adjacent_vertices(mesh, a)) {
                if (adjacent == b) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Checks whether two faces share an edge.
         *
         * @param mesh Mesh to query.
         * @param a First face.
         * @param b Second face.
         * @return True when the faces are edge-adjacent.
         */
        [[nodiscard]] static bool are_faces_adjacent(const LEM& mesh, FaceHandle a, FaceHandle b)
        {
            if (!mesh.is_valid(a) || !mesh.is_valid(b) || a == b) {
                return false;
            }

            for (FaceHandle adjacent : adjacent_faces(mesh, a)) {
                if (adjacent == b) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Checks whether an edge has fewer than two radial loops.
         *
         * @param mesh Mesh to query.
         * @param edge Edge to test.
         * @return True when the edge is on a boundary.
         */
        [[nodiscard]] static bool is_boundary_edge(const LEM& mesh, EdgeHandle edge)
        {
            return TopologyTraversal::is_boundary_edge(mesh, edge);
        }

        /**
         * @brief Checks whether an edge is used by at most two loops.
         *
         * @param mesh Mesh to query.
         * @param edge Edge to test.
         * @return True when the edge is manifold under LEM radial rules.
         */
        [[nodiscard]] static bool is_manifold_edge(const LEM& mesh, EdgeHandle edge)
        {
            return TopologyTraversal::is_manifold_edge(mesh, edge);
        }

        /**
         * @brief Checks whether a vertex has no incident edges.
         *
         * @param mesh Mesh to query.
         * @param vertex Vertex to test.
         * @return True when the vertex is valid and loose.
         */
        [[nodiscard]] static bool is_loose_vertex(const LEM& mesh, VertexHandle vertex)
        {
            return mesh.is_valid(vertex) && TopologyTraversal::vertex_edges(mesh, vertex).empty();
        }

        /**
         * @brief Checks whether an edge has no radial loops.
         *
         * @param mesh Mesh to query.
         * @param edge Edge to test.
         * @return True when the edge is valid and loose.
         */
        [[nodiscard]] static bool is_loose_edge(const LEM& mesh, EdgeHandle edge)
        {
            return mesh.is_valid(edge) && TopologyTraversal::edge_loops(mesh, edge).empty();
        }

    private:
        template <typename Handle>
        /**
         * @brief Checks whether a handle list already contains a handle.
         *
         * @tparam Handle Handle type stored in the list.
         * @param handles Handle list to search.
         * @param handle Handle to find.
         * @return True when the handle is present.
         */
        [[nodiscard]] static bool contains(const std::vector<Handle>& handles, Handle handle)
        {
            return std::find(handles.begin(), handles.end(), handle) != handles.end();
        }
    };

}
