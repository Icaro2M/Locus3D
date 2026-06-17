/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace locus::kernel::geometry
{
    /**
     * @brief Builds renderable line meshes from editable topology.
     */
    class WireframeBuilder
    {
    public:
        /**
         * @brief Builds a wireframe for all active mesh edges.
         *
         * @param mesh Source editable mesh.
         * @return Render mesh containing line primitives.
         */
        [[nodiscard]] static RenderMesh build(const LEM& mesh)
        {
            RenderMesh result;
            build_into(mesh, result);
            return result;
        }

        /**
         * @brief Builds a wireframe for all active mesh edges into an output mesh.
         *
         * @param mesh Source editable mesh.
         * @param output Render mesh that receives line primitives.
         */
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

        /**
         * @brief Adds one edge as a line primitive.
         *
         * @param mesh Source editable mesh.
         * @param edgeHandle Edge to append.
         * @param output Render mesh that receives the line.
         */
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

        /**
         * @brief Builds a wireframe for a single face boundary.
         *
         * @param mesh Source editable mesh.
         * @param faceHandle Face to convert.
         * @return Render mesh containing the face boundary lines.
         */
        [[nodiscard]] static RenderMesh build_face_wireframe(const LEM& mesh, FaceHandle faceHandle)
        {
            RenderMesh result;
            build_face_wireframe_into(mesh, faceHandle, result);
            return result;
        }

        /**
         * @brief Builds a wireframe for a single face boundary into an output mesh.
         *
         * @param mesh Source editable mesh.
         * @param faceHandle Face to convert.
         * @param output Render mesh that receives line primitives.
         */
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

        /**
         * @brief Builds a wireframe containing only boundary edges.
         *
         * @param mesh Source editable mesh.
         * @return Render mesh containing boundary line primitives.
         */
        [[nodiscard]] static RenderMesh build_boundary_wireframe(const LEM& mesh)
        {
            RenderMesh result;
            build_boundary_wireframe_into(mesh, result);
            return result;
        }

        /**
         * @brief Builds a boundary-only wireframe into an output mesh.
         *
         * @param mesh Source editable mesh.
         * @param output Render mesh that receives boundary line primitives.
         */
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
