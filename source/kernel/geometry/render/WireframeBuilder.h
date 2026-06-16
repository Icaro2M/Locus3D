#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace locus::kernel::geometry
{
    class WireframeBuilder
    {
    public:
        [[nodiscard]] static RenderMesh build(const LEM& mesh)
        {
            RenderMesh result;
            build_into(mesh, result);
            return result;
        }

        static void build_into(const LEM& mesh, RenderMesh& output)
        {
            output.clear();
            output.reserve_vertices(mesh.edge_count() * 2);
            output.reserve_lines(mesh.edge_count());

            for (EdgeHandle edgeHandle : TopologyTraversal::edges(mesh))
            {
                add_edge(mesh, edgeHandle, output);
            }
        }

        static void add_edge(const LEM& mesh, EdgeHandle edgeHandle, RenderMesh& output)
        {
            if (!mesh.is_valid(edgeHandle))
            {
                return;
            }

            const Edge& edge = mesh.edge(edgeHandle);
            if (!mesh.is_valid(edge.vertexA) || !mesh.is_valid(edge.vertexB))
            {
                return;
            }

            const Vertex& vertexA = mesh.vertex(edge.vertexA);
            const Vertex& vertexB = mesh.vertex(edge.vertexB);

            const RenderIndex indexA = output.add_vertex(vertexA.position);
            const RenderIndex indexB = output.add_vertex(vertexB.position);

            output.add_line(indexA, indexB);
        }

        [[nodiscard]] static RenderMesh build_face_wireframe(const LEM& mesh, FaceHandle faceHandle)
        {
            RenderMesh result;
            build_face_wireframe_into(mesh, faceHandle, result);
            return result;
        }

        static void build_face_wireframe_into(const LEM& mesh, FaceHandle faceHandle, RenderMesh& output)
        {
            output.clear();

            if (!mesh.is_valid(faceHandle))
            {
                return;
            }

            const std::vector<EdgeHandle> edges = TopologyTraversal::face_edges(mesh, faceHandle);

            output.reserve_vertices(edges.size() * 2);
            output.reserve_lines(edges.size());

            for (EdgeHandle edgeHandle : edges)
            {
                add_edge(mesh, edgeHandle, output);
            }
        }

        [[nodiscard]] static RenderMesh build_boundary_wireframe(const LEM& mesh)
        {
            RenderMesh result;
            build_boundary_wireframe_into(mesh, result);
            return result;
        }

        static void build_boundary_wireframe_into(const LEM& mesh, RenderMesh& output)
        {
            output.clear();

            for (EdgeHandle edgeHandle : TopologyTraversal::edges(mesh))
            {
                if (TopologyTraversal::is_boundary_edge(mesh, edgeHandle))
                {
                    add_edge(mesh, edgeHandle, output);
                }
            }
        }
    };
}