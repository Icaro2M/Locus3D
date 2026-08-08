/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/spatial/BVHBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/mesh/AnalysisMesh.h"

#include <cstddef>
#include <limits>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Builds the derived triangulated representation used by
     * manufacturing analyzers.
     *
     * MeshTriangulator defines the canonical polygon triangulation. Spatial
     * acceleration is then built from those exact derived triangles instead
     * of independently triangulating the source LEM.
     */
    class AnalysisMeshBuilder {
    public:
        /**
         * @brief Builds a new analysis mesh from editable geometry.
         *
         * @param mesh Source authoritative LEM.
         * @return Complete derived analysis representation.
         */
        [[nodiscard]] static AnalysisMesh build(
            const geometry::LEM& mesh)
        {
            AnalysisMesh output;
            build_into(mesh, output);
            return output;
        }

        /**
         * @brief Rebuilds an existing analysis mesh from editable geometry.
         *
         * Existing triangulation, mapping, bounds and acceleration structures
         * are discarded.
         *
         * @param mesh Source authoritative LEM.
         * @param output Analysis mesh receiving rebuilt data.
         */
        static void build_into(
            const geometry::LEM& mesh,
            AnalysisMesh& output)
        {
            output.clear();

            for (geometry::FaceHandle faceHandle :
            geometry::TopologyTraversal::faces(mesh)) {

                if (!mesh.is_valid(faceHandle)) {
                    continue;
                }

                append_face(
                    mesh,
                    faceHandle,
                    output);
            }

            build_acceleration(output);
        }

    private:
        /**
         * @brief Triangulates and appends one editable face.
         *
         * A temporary RenderMesh is used solely as the output carrier of the
         * kernel's canonical MeshTriangulator. Manufacturing-owned data is
         * copied immediately into AnalysisMesh.
         */
        static void append_face(
            const geometry::LEM& mesh,
            geometry::FaceHandle faceHandle,
            AnalysisMesh& output)
        {
            geometry::RenderMesh triangulatedFace;

            geometry::MeshTriangulator::triangulate_face_into(
                mesh,
                faceHandle,
                triangulatedFace);

            if (triangulatedFace.triangles.empty()) {
                return;
            }

            const std::size_t maximumIndex =
                static_cast<std::size_t>(
                    std::numeric_limits<AnalysisIndex>::max());

            const std::size_t baseVertex =
                output.vertex_count();

            if (baseVertex > maximumIndex) {
                return;
            }

            if (triangulatedFace.vertices.size() >
                maximumIndex - baseVertex) {
                return;
            }

            for (const geometry::RenderVertex& vertex :
                triangulatedFace.vertices) {

                output.add_vertex(
                    AnalysisVertex{
                        vertex.position,
                        vertex.normal
                    });
            }

            for (const geometry::RenderTriangle& triangle :
                triangulatedFace.triangles) {

                if (triangle.a >=
                    triangulatedFace.vertices.size() ||
                    triangle.b >=
                    triangulatedFace.vertices.size() ||
                    triangle.c >=
                    triangulatedFace.vertices.size()) {
                    continue;
                }

                const std::size_t a =
                    baseVertex +
                    static_cast<std::size_t>(
                        triangle.a);

                const std::size_t b =
                    baseVertex +
                    static_cast<std::size_t>(
                        triangle.b);

                const std::size_t c =
                    baseVertex +
                    static_cast<std::size_t>(
                        triangle.c);

                if (a > maximumIndex ||
                    b > maximumIndex ||
                    c > maximumIndex) {
                    continue;
                }

                output.add_triangle(
                    AnalysisTriangle{
                        static_cast<AnalysisIndex>(a),
                        static_cast<AnalysisIndex>(b),
                        static_cast<AnalysisIndex>(c)
                    },
                    faceHandle);
            }
        }

        /**
         * @brief Builds spatial acceleration from canonical analysis
         * triangles.
         *
         * Source FaceHandles are copied from MeshHandleMapping, preserving
         * direct correspondence with editable LEM faces after BVH triangle
         * reordering.
         */
        static void build_acceleration(
            AnalysisMesh& output)
        {
            if (output.triangle_count() == 0) {
                return;
            }

            std::vector<geometry::BVHTriangle> triangles;

            triangles.reserve(
                output.triangle_count());

            for (std::size_t index = 0;
                index < output.triangle_count();
                ++index) {

                const AnalysisTriangle& triangle =
                    output.triangles_[index];

                if (triangle.a >= output.vertices_.size() ||
                    triangle.b >= output.vertices_.size() ||
                    triangle.c >= output.vertices_.size()) {
                    continue;
                }

                const AnalysisVertex& a =
                    output.vertices_[triangle.a];

                const AnalysisVertex& b =
                    output.vertices_[triangle.b];

                const AnalysisVertex& c =
                    output.vertices_[triangle.c];

                const geometry::FaceHandle sourceFace =
                    output.mapping_
                    .face_for_triangle(index);

                if (!sourceFace.is_valid()) {
                    continue;
                }

                geometry::BVHTriangle bvhTriangle;

                bvhTriangle.face = sourceFace;

                bvhTriangle.a = a.position;
                bvhTriangle.b = b.position;
                bvhTriangle.c = c.position;

                bvhTriangle.normal = a.normal;

                triangles.push_back(bvhTriangle);
            }

            geometry::BVHBuilder::build_into(
                triangles,
                output.bvh_);
        }
    };

}