/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace locus::kernel::geometry
{
    /**
     * @brief Utility class for querying LEM topology relationships.
     */
    class TopologyTraversal
    {
    public:
        /**
         * @brief Returns all active vertices in the mesh.
         *
         * @param mesh Mesh to inspect.
         * @return Handles for active vertices.
         */
        [[nodiscard]] static std::vector<VertexHandle> vertices(const LEM& mesh)
        {
            std::vector<VertexHandle> result;
            result.reserve(mesh.vertex_count());

            for (std::size_t index = 0; index < mesh.vertex_count(); ++index)
            {
                VertexHandle handle(static_cast<IdValue>(index));
                if (mesh.is_valid(handle))
                {
                    result.push_back(handle);
                }
            }

            return result;
        }

        /**
         * @brief Returns all active edges in the mesh.
         *
         * @param mesh Mesh to inspect.
         * @return Handles for active edges.
         */
        [[nodiscard]] static std::vector<EdgeHandle> edges(const LEM& mesh)
        {
            std::vector<EdgeHandle> result;
            result.reserve(mesh.edge_count());

            for (std::size_t index = 0; index < mesh.edge_count(); ++index)
            {
                EdgeHandle handle(static_cast<IdValue>(index));
                if (mesh.is_valid(handle))
                {
                    result.push_back(handle);
                }
            }

            return result;
        }

        /**
         * @brief Returns all active loops in the mesh.
         *
         * @param mesh Mesh to inspect.
         * @return Handles for active loops.
         */
        [[nodiscard]] static std::vector<LoopHandle> loops(const LEM& mesh)
        {
            std::vector<LoopHandle> result;
            result.reserve(mesh.loop_count());

            for (std::size_t index = 0; index < mesh.loop_count(); ++index)
            {
                LoopHandle handle(static_cast<IdValue>(index));
                if (mesh.is_valid(handle))
                {
                    result.push_back(handle);
                }
            }

            return result;
        }

        /**
         * @brief Returns all active faces in the mesh.
         *
         * @param mesh Mesh to inspect.
         * @return Handles for active faces.
         */
        [[nodiscard]] static std::vector<FaceHandle> faces(const LEM& mesh)
        {
            std::vector<FaceHandle> result;
            result.reserve(mesh.face_count());

            for (std::size_t index = 0; index < mesh.face_count(); ++index)
            {
                FaceHandle handle(static_cast<IdValue>(index));
                if (mesh.is_valid(handle))
                {
                    result.push_back(handle);
                }
            }

            return result;
        }

        /**
         * @brief Returns the boundary loops of a face.
         *
         * @param mesh Mesh to inspect.
         * @param face Face handle.
         * @return Ordered loop handles around the face, or empty when invalid.
         */
        [[nodiscard]] static std::vector<LoopHandle> face_loops(const LEM& mesh, FaceHandle face)
        {
            if (!mesh.is_valid(face))
            {
                return {};
            }

            return mesh.face_loops(face);
        }

        /**
         * @brief Returns the vertices used by a face boundary.
         *
         * @param mesh Mesh to inspect.
         * @param face Face handle.
         * @return Ordered vertex handles around the face.
         */
        [[nodiscard]] static std::vector<VertexHandle> face_vertices(const LEM& mesh, FaceHandle face)
        {
            std::vector<VertexHandle> result;

            for (LoopHandle loopHandle : face_loops(mesh, face))
            {
                const Loop& loop = mesh.loop(loopHandle);
                if (mesh.is_valid(loop.vertex))
                {
                    result.push_back(loop.vertex);
                }
            }

            return result;
        }

        /**
         * @brief Returns the unique edges used by a face boundary.
         *
         * @param mesh Mesh to inspect.
         * @param face Face handle.
         * @return Edge handles used by the face.
         */
        [[nodiscard]] static std::vector<EdgeHandle> face_edges(const LEM& mesh, FaceHandle face)
        {
            std::vector<EdgeHandle> result;

            for (LoopHandle loopHandle : face_loops(mesh, face))
            {
                const Loop& loop = mesh.loop(loopHandle);
                if (mesh.is_valid(loop.edge) && !contains(result, loop.edge))
                {
                    result.push_back(loop.edge);
                }
            }

            return result;
        }

        /**
         * @brief Returns the endpoint vertices of an edge.
         *
         * @param mesh Mesh to inspect.
         * @param edge Edge handle.
         * @return Pair of vertex handles, or invalid handles when the edge is invalid.
         */
        [[nodiscard]] static std::array<VertexHandle, 2> edge_vertices(const LEM& mesh, EdgeHandle edge)
        {
            if (!mesh.is_valid(edge))
            {
                return {};
            }

            const Edge& edgeElement = mesh.edge(edge);
            return { edgeElement.vertexA, edgeElement.vertexB };
        }

        /**
         * @brief Returns all loops in the radial cycle of an edge.
         *
         * @param mesh Mesh to inspect.
         * @param edge Edge handle.
         * @return Loop handles around the edge.
         * @note Traversal stops early if the radial cycle is broken or repeats unexpectedly.
         */
        [[nodiscard]] static std::vector<LoopHandle> edge_loops(const LEM& mesh, EdgeHandle edge)
        {
            if (!mesh.is_valid(edge))
            {
                return {};
            }

            const Edge& edgeElement = mesh.edge(edge);
            if (!mesh.is_valid(edgeElement.loop))
            {
                return {};
            }

            std::vector<LoopHandle> result;
            LoopHandle first = edgeElement.loop;
            LoopHandle current = first;

            do
            {
                if (!mesh.is_valid(current) || contains(result, current))
                {
                    return result;
                }

                result.push_back(current);

                const Loop& loop = mesh.loop(current);
                current = loop.radialNext;
            } while (current != first);

            return result;
        }

        /**
         * @brief Returns the unique faces adjacent to an edge.
         *
         * @param mesh Mesh to inspect.
         * @param edge Edge handle.
         * @return Face handles attached to the edge radial cycle.
         */
        [[nodiscard]] static std::vector<FaceHandle> edge_faces(const LEM& mesh, EdgeHandle edge)
        {
            std::vector<FaceHandle> result;

            for (LoopHandle loopHandle : edge_loops(mesh, edge))
            {
                const Loop& loop = mesh.loop(loopHandle);
                if (mesh.is_valid(loop.face) && !contains(result, loop.face))
                {
                    result.push_back(loop.face);
                }
            }

            return result;
        }

        /**
         * @brief Returns all edges incident to a vertex.
         *
         * @param mesh Mesh to inspect.
         * @param vertex Vertex handle.
         * @return Incident edge handles.
         */
        [[nodiscard]] static std::vector<EdgeHandle> vertex_edges(const LEM& mesh, VertexHandle vertex)
        {
            if (!mesh.is_valid(vertex))
            {
                return {};
            }

            const Vertex& vertexElement = mesh.vertex(vertex);
            if (!mesh.is_valid(vertexElement.edge))
            {
                return {};
            }

            std::vector<EdgeHandle> result;
            std::vector<EdgeHandle> pending;
            pending.push_back(vertexElement.edge);

            while (!pending.empty())
            {
                const EdgeHandle edgeHandle = pending.back();
                pending.pop_back();

                if (!mesh.is_valid(edgeHandle) || contains(result, edgeHandle))
                {
                    continue;
                }

                const Edge& edge = mesh.edge(edgeHandle);
                if (edge.vertexA != vertex && edge.vertexB != vertex)
                {
                    continue;
                }

                result.push_back(edgeHandle);

                for (LoopHandle loopHandle : edge_loops(mesh, edgeHandle))
                {
                    const Loop& loop = mesh.loop(loopHandle);

                    if (loop.vertex == vertex && mesh.is_valid(loop.previous))
                    {
                        const Loop& previous = mesh.loop(loop.previous);
                        if (mesh.is_valid(previous.edge) && !contains(result, previous.edge))
                        {
                            pending.push_back(previous.edge);
                        }
                    }

                    if (mesh.is_valid(loop.next))
                    {
                        const Loop& next = mesh.loop(loop.next);
                        if (next.vertex == vertex && mesh.is_valid(next.edge) && !contains(result, next.edge))
                        {
                            pending.push_back(next.edge);
                        }
                    }
                }
            }

            return result;
        }

        /**
         * @brief Returns all loops that reference a vertex.
         *
         * @param mesh Mesh to inspect.
         * @param vertex Vertex handle.
         * @return Loop handles using the vertex.
         */
        [[nodiscard]] static std::vector<LoopHandle> vertex_loops(const LEM& mesh, VertexHandle vertex)
        {
            if (!mesh.is_valid(vertex))
            {
                return {};
            }

            std::vector<LoopHandle> result;

            for (EdgeHandle edgeHandle : vertex_edges(mesh, vertex))
            {
                for (LoopHandle loopHandle : edge_loops(mesh, edgeHandle))
                {
                    const Loop& loop = mesh.loop(loopHandle);
                    if (loop.vertex == vertex && !contains(result, loopHandle))
                    {
                        result.push_back(loopHandle);
                    }
                }
            }

            return result;
        }

        /**
         * @brief Returns the unique faces adjacent to a vertex.
         *
         * @param mesh Mesh to inspect.
         * @param vertex Vertex handle.
         * @return Face handles using the vertex.
         */
        [[nodiscard]] static std::vector<FaceHandle> vertex_faces(const LEM& mesh, VertexHandle vertex)
        {
            std::vector<FaceHandle> result;

            for (LoopHandle loopHandle : vertex_loops(mesh, vertex))
            {
                const Loop& loop = mesh.loop(loopHandle);
                if (mesh.is_valid(loop.face) && !contains(result, loop.face))
                {
                    result.push_back(loop.face);
                }
            }

            return result;
        }

        /**
         * @brief Returns vertices connected to a vertex by an edge.
         *
         * @param mesh Mesh to inspect.
         * @param vertex Vertex handle.
         * @return Unique adjacent vertex handles.
         */
        [[nodiscard]] static std::vector<VertexHandle> adjacent_vertices(const LEM& mesh, VertexHandle vertex)
        {
            std::vector<VertexHandle> result;

            for (EdgeHandle edgeHandle : vertex_edges(mesh, vertex))
            {
                const Edge& edge = mesh.edge(edgeHandle);
                VertexHandle adjacent = edge.vertexA == vertex ? edge.vertexB : edge.vertexA;

                if (mesh.is_valid(adjacent) && !contains(result, adjacent))
                {
                    result.push_back(adjacent);
                }
            }

            return result;
        }

        /**
         * @brief Checks whether an edge has fewer than two radial loops.
         *
         * @param mesh Mesh to inspect.
         * @param edge Edge handle.
         * @return True when the edge is valid and lies on a boundary.
         */
        [[nodiscard]] static bool is_boundary_edge(const LEM& mesh, EdgeHandle edge)
        {
            if (!mesh.is_valid(edge))
            {
                return false;
            }

            return edge_loops(mesh, edge).size() < 2;
        }

        /**
         * @brief Checks whether an edge is used by at most two loops.
         *
         * @param mesh Mesh to inspect.
         * @param edge Edge handle.
         * @return True when the edge is valid and manifold.
         */
        [[nodiscard]] static bool is_manifold_edge(const LEM& mesh, EdgeHandle edge)
        {
            if (!mesh.is_valid(edge))
            {
                return false;
            }

            return edge_loops(mesh, edge).size() <= 2;
        }

    private:
        template <typename Handle>
        [[nodiscard]] static bool contains(const std::vector<Handle>& handles, Handle handle)
        {
            return std::find(handles.begin(), handles.end(), handle) != handles.end();
        }
    };
}
