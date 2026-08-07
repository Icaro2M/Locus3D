/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyTraversal.h"
#include "kernel/manufacturing/mesh/AnalysisMesh.h"

#include <cstddef>
#include <limits>

namespace locus::kernel::manufacturing {

    /**
     * @brief Builds the derived triangulated representation used by
     * manufacturing analyzers.
     *
     * Triangulation is delegated to geometry::MeshTriangulator so rendering,
     * export-oriented geometry helpers, and manufacturing analysis do not
     * independently implement polygon triangulation rules.
     *
     * Each generated triangle is mapped back to the original LEM FaceHandle.
     */
    class AnalysisMeshBuilder {
    public:
        /**
         * @brief Builds a new analysis mesh from editable geometry.
         *
         * @param mesh Source LEM.
         * @return Derived triangulated analysis representation.
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
         * Existing derived geometry and mappings are discarded.
         *
         * @param mesh Source authoritative LEM.
         * @param output Analysis mesh that receives the rebuilt data.
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

                append_face(mesh, faceHandle, output);
            }
        }

    private:
        /**
         * @brief Triangulates and appends one editable face.
         *
         * A temporary RenderMesh is intentionally used only as the output
         * carrier of the kernel's canonical polygon triangulator. Render
         * primitives are immediately converted into manufacturing-owned
         * AnalysisVertex and AnalysisTriangle data.
         *
         * @param mesh Source editable mesh.
         * @param faceHandle Face to triangulate.
         * @param output Analysis mesh receiving generated geometry.
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

            const std::size_t baseVertex = output.vertex_count();

            if (baseVertex >
                static_cast<std::size_t>(
                    std::numeric_limits<AnalysisIndex>::max())) {
                return;
            }

            for (const geometry::RenderVertex& vertex :
                triangulatedFace.vertices) {

                if (output.vertex_count() >=
                    static_cast<std::size_t>(
                        std::numeric_limits<AnalysisIndex>::max())) {
                    return;
                }

                output.add_vertex(
                    AnalysisVertex{
                        vertex.position,
                        vertex.normal
                    });
            }

            for (const geometry::RenderTriangle& triangle :
                triangulatedFace.triangles) {

                if (triangle.a >= triangulatedFace.vertices.size() ||
                    triangle.b >= triangulatedFace.vertices.size() ||
                    triangle.c >= triangulatedFace.vertices.size()) {
                    continue;
                }

                const std::size_t a =
                    baseVertex + static_cast<std::size_t>(triangle.a);

                const std::size_t b =
                    baseVertex + static_cast<std::size_t>(triangle.b);

                const std::size_t c =
                    baseVertex + static_cast<std::size_t>(triangle.c);

                const std::size_t maximumIndex =
                    static_cast<std::size_t>(
                        std::numeric_limits<AnalysisIndex>::max());

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
    };

}