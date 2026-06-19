#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <vector>

namespace locus::kernel::geometry {

    class AdjacencyQuery {
    public:
        [[nodiscard]] static std::vector<EdgeHandle> vertex_edges(const LEM& mesh, VertexHandle vertex)
        {
            return TopologyTraversal::vertex_edges(mesh, vertex);
        }

        [[nodiscard]] static std::vector<LoopHandle> vertex_loops(const LEM& mesh, VertexHandle vertex)
        {
            return TopologyTraversal::vertex_loops(mesh, vertex);
        }

        [[nodiscard]] static std::vector<FaceHandle> vertex_faces(const LEM& mesh, VertexHandle vertex)
        {
            return TopologyTraversal::vertex_faces(mesh, vertex);
        }

        [[nodiscard]] static std::vector<VertexHandle> adjacent_vertices(const LEM& mesh, VertexHandle vertex)
        {
            return TopologyTraversal::adjacent_vertices(mesh, vertex);
        }

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

        [[nodiscard]] static std::vector<LoopHandle> edge_loops(const LEM& mesh, EdgeHandle edge)
        {
            return TopologyTraversal::edge_loops(mesh, edge);
        }

        [[nodiscard]] static std::vector<FaceHandle> edge_faces(const LEM& mesh, EdgeHandle edge)
        {
            return TopologyTraversal::edge_faces(mesh, edge);
        }

        [[nodiscard]] static std::vector<LoopHandle> face_loops(const LEM& mesh, FaceHandle face)
        {
            return TopologyTraversal::face_loops(mesh, face);
        }

        [[nodiscard]] static std::vector<VertexHandle> face_vertices(const LEM& mesh, FaceHandle face)
        {
            return TopologyTraversal::face_vertices(mesh, face);
        }

        [[nodiscard]] static std::vector<EdgeHandle> face_edges(const LEM& mesh, FaceHandle face)
        {
            return TopologyTraversal::face_edges(mesh, face);
        }

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

        [[nodiscard]] static bool is_boundary_edge(const LEM& mesh, EdgeHandle edge)
        {
            return TopologyTraversal::is_boundary_edge(mesh, edge);
        }

        [[nodiscard]] static bool is_manifold_edge(const LEM& mesh, EdgeHandle edge)
        {
            return TopologyTraversal::is_manifold_edge(mesh, edge);
        }

        [[nodiscard]] static bool is_loose_vertex(const LEM& mesh, VertexHandle vertex)
        {
            return mesh.is_valid(vertex) && TopologyTraversal::vertex_edges(mesh, vertex).empty();
        }

        [[nodiscard]] static bool is_loose_edge(const LEM& mesh, EdgeHandle edge)
        {
            return mesh.is_valid(edge) && TopologyTraversal::edge_loops(mesh, edge).empty();
        }

    private:
        template <typename Handle>
        [[nodiscard]] static bool contains(const std::vector<Handle>& handles, Handle handle)
        {
            return std::find(handles.begin(), handles.end(), handle) != handles.end();
        }
    };

}