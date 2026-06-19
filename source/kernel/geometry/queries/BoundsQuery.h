/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/math/Bounds.h"

namespace locus::kernel::geometry {

    /**
     * @brief Computes axis-aligned bounds for mesh elements and selections.
     */
    class BoundsQuery {
    public:
        /**
         * @brief Computes bounds for all visible vertices in a mesh.
         *
         * @param mesh Mesh to query.
         * @return Bounds enclosing every non-hidden vertex.
         */
        [[nodiscard]] static math::Bounds mesh_bounds(const LEM& mesh)
        {
            math::Bounds bounds = math::Bounds::empty();

            for (VertexHandle vertexHandle : TopologyTraversal::vertices(mesh)) {
                const Vertex& vertex = mesh.vertex(vertexHandle);
                if (!vertex.hidden) {
                    bounds.expand(vertex.position);
                }
            }

            return bounds;
        }

        /**
         * @brief Computes bounds for selected visible vertices, edges, and faces.
         *
         * @param mesh Mesh to query.
         * @return Bounds enclosing selected non-hidden geometry.
         */
        [[nodiscard]] static math::Bounds selected_bounds(const LEM& mesh)
        {
            math::Bounds bounds = math::Bounds::empty();

            for (VertexHandle vertexHandle : TopologyTraversal::vertices(mesh)) {
                const Vertex& vertex = mesh.vertex(vertexHandle);
                if (vertex.selected && !vertex.hidden) {
                    bounds.expand(vertex.position);
                }
            }

            for (EdgeHandle edgeHandle : TopologyTraversal::edges(mesh)) {
                const Edge& edge = mesh.edge(edgeHandle);
                if (!edge.selected || edge.hidden) {
                    continue;
                }

                expand_edge(mesh, edgeHandle, bounds);
            }

            for (FaceHandle faceHandle : TopologyTraversal::faces(mesh)) {
                const Face& face = mesh.face(faceHandle);
                if (!face.selected || face.hidden) {
                    continue;
                }

                expand_face(mesh, faceHandle, bounds);
            }

            return bounds;
        }

        /**
         * @brief Computes bounds for a single visible vertex.
         *
         * @param mesh Mesh containing the vertex.
         * @param vertexHandle Vertex to query.
         * @return Bounds containing the vertex position, or empty bounds.
         */
        [[nodiscard]] static math::Bounds vertex_bounds(const LEM& mesh, VertexHandle vertexHandle)
        {
            math::Bounds bounds = math::Bounds::empty();

            if (!mesh.is_valid(vertexHandle)) {
                return bounds;
            }

            const Vertex& vertex = mesh.vertex(vertexHandle);
            if (!vertex.hidden) {
                bounds.expand(vertex.position);
            }

            return bounds;
        }

        /**
         * @brief Computes bounds for a single visible edge.
         *
         * @param mesh Mesh containing the edge.
         * @param edgeHandle Edge to query.
         * @return Bounds enclosing the edge endpoints, or empty bounds.
         */
        [[nodiscard]] static math::Bounds edge_bounds(const LEM& mesh, EdgeHandle edgeHandle)
        {
            math::Bounds bounds = math::Bounds::empty();
            expand_edge(mesh, edgeHandle, bounds);
            return bounds;
        }

        /**
         * @brief Computes bounds for a single visible face.
         *
         * @param mesh Mesh containing the face.
         * @param faceHandle Face to query.
         * @return Bounds enclosing the face vertices, or empty bounds.
         */
        [[nodiscard]] static math::Bounds face_bounds(const LEM& mesh, FaceHandle faceHandle)
        {
            math::Bounds bounds = math::Bounds::empty();
            expand_face(mesh, faceHandle, bounds);
            return bounds;
        }

    private:
        /**
         * @brief Expands bounds with a visible vertex position.
         *
         * @param mesh Mesh containing the vertex.
         * @param vertexHandle Vertex to include.
         * @param bounds Bounds to expand.
         */
        static void expand_vertex(const LEM& mesh, VertexHandle vertexHandle, math::Bounds& bounds)
        {
            if (!mesh.is_valid(vertexHandle)) {
                return;
            }

            const Vertex& vertex = mesh.vertex(vertexHandle);
            if (vertex.hidden) {
                return;
            }

            bounds.expand(vertex.position);
        }

        /**
         * @brief Expands bounds with a visible edge's endpoints.
         *
         * @param mesh Mesh containing the edge.
         * @param edgeHandle Edge to include.
         * @param bounds Bounds to expand.
         */
        static void expand_edge(const LEM& mesh, EdgeHandle edgeHandle, math::Bounds& bounds)
        {
            if (!mesh.is_valid(edgeHandle)) {
                return;
            }

            const Edge& edge = mesh.edge(edgeHandle);
            if (edge.hidden) {
                return;
            }

            expand_vertex(mesh, edge.vertexA, bounds);
            expand_vertex(mesh, edge.vertexB, bounds);
        }

        /**
         * @brief Expands bounds with a visible face's vertices.
         *
         * @param mesh Mesh containing the face.
         * @param faceHandle Face to include.
         * @param bounds Bounds to expand.
         */
        static void expand_face(const LEM& mesh, FaceHandle faceHandle, math::Bounds& bounds)
        {
            if (!mesh.is_valid(faceHandle)) {
                return;
            }

            const Face& face = mesh.face(faceHandle);
            if (face.hidden) {
                return;
            }

            for (VertexHandle vertexHandle : TopologyTraversal::face_vertices(mesh, faceHandle)) {
                expand_vertex(mesh, vertexHandle, bounds);
            }
        }
    };

}
