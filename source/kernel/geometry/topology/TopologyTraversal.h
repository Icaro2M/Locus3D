#pragma once

#include "kernel/geometry/mesh/LEM.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace locus::kernel::geometry
{
    class TopologyTraversal
    {
    public:
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

        [[nodiscard]] static std::vector<LoopHandle> face_loops(const LEM& mesh, FaceHandle face)
        {
            if (!mesh.is_valid(face))
            {
                return {};
            }

            return mesh.face_loops(face);
        }

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

        [[nodiscard]] static std::array<VertexHandle, 2> edge_vertices(const LEM& mesh, EdgeHandle edge)
        {
            if (!mesh.is_valid(edge))
            {
                return {};
            }

            const Edge& edgeElement = mesh.edge(edge);
            return { edgeElement.vertexA, edgeElement.vertexB };
        }

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

        [[nodiscard]] static std::vector<EdgeHandle> vertex_edges(const LEM& mesh, VertexHandle vertex)
        {
            if (!mesh.is_valid(vertex))
            {
                return {};
            }

            std::vector<EdgeHandle> result;

            for (std::size_t index = 0; index < mesh.edge_count(); ++index)
            {
                EdgeHandle edgeHandle(static_cast<IdValue>(index));
                if (!mesh.is_valid(edgeHandle))
                {
                    continue;
                }

                const Edge& edge = mesh.edge(edgeHandle);
                if (edge.vertexA == vertex || edge.vertexB == vertex)
                {
                    result.push_back(edgeHandle);
                }
            }

            return result;
        }

        [[nodiscard]] static std::vector<LoopHandle> vertex_loops(const LEM& mesh, VertexHandle vertex)
        {
            if (!mesh.is_valid(vertex))
            {
                return {};
            }

            std::vector<LoopHandle> result;

            for (std::size_t index = 0; index < mesh.loop_count(); ++index)
            {
                LoopHandle loopHandle(static_cast<IdValue>(index));
                if (!mesh.is_valid(loopHandle))
                {
                    continue;
                }

                const Loop& loop = mesh.loop(loopHandle);
                if (loop.vertex == vertex)
                {
                    result.push_back(loopHandle);
                }
            }

            return result;
        }

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

        [[nodiscard]] static bool is_boundary_edge(const LEM& mesh, EdgeHandle edge)
        {
            if (!mesh.is_valid(edge))
            {
                return false;
            }

            return edge_loops(mesh, edge).size() < 2;
        }

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