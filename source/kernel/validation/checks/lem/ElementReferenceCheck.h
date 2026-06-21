/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Id.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/validation/core/IValidationCheck.h"
#include "kernel/validation/core/ValidationSeverity.h"

#include <cstddef>
#include <string_view>

namespace locus::kernel::validation {

    /**
     * @brief Validates semantic consistency between referenced LEM elements.
     */
    class ElementReferenceCheck final : public IValidationCheck {
    public:
        /**
         * @brief Returns the stable check name.
         *
         * @return Check name used in diagnostics.
         */
        [[nodiscard]] std::string_view name() const override
        {
            return "ElementReferenceCheck";
        }

        /**
         * @brief Executes element reference validation.
         *
         * @param context Validation input data.
         * @param report Report that receives produced diagnostics.
         */
        void validate(const ValidationContext& context, ValidationReport& report) const override
        {
            if (!context.has_mesh()) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "validation.missing_mesh",
                    std::string(name()),
                    "mesh",
                    Id{},
                    "Validation context does not contain a mesh."
                );
                return;
            }

            const geometry::LEM& mesh = *context.mesh;

            validate_vertices(mesh, report);
            validate_edges(mesh, report);
            validate_loops(mesh, report);
            validate_faces(mesh, report);
        }

    private:
        static void validate_vertices(const geometry::LEM& mesh, ValidationReport& report)
        {
            const auto& vertices = mesh.vertices();

            for (std::size_t index = 0; index < vertices.size(); ++index) {
                const geometry::Vertex& vertex = vertices[index];

                if (vertex.deleted || vertex.edge.is_invalid() || !mesh.is_valid(vertex.edge)) {
                    continue;
                }

                const geometry::Edge& edge = mesh.edge(vertex.edge);
                const geometry::VertexHandle vertexHandle(static_cast<IdValue>(index));

                if (edge.vertexA != vertexHandle && edge.vertexB != vertexHandle) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.reference.vertex_edge_not_incident",
                        "ElementReferenceCheck",
                        "vertex",
                        Id(static_cast<IdValue>(index)),
                        "Vertex incident edge does not reference the vertex."
                    );
                }
            }
        }

        static void validate_edges(const geometry::LEM& mesh, ValidationReport& report)
        {
            const auto& edges = mesh.edges();

            for (std::size_t index = 0; index < edges.size(); ++index) {
                const geometry::Edge& edge = edges[index];

                if (edge.deleted) {
                    continue;
                }

                if (edge.vertexA == edge.vertexB) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.reference.edge_same_vertices",
                        "ElementReferenceCheck",
                        "edge",
                        Id(static_cast<IdValue>(index)),
                        "Edge references the same vertex twice."
                    );
                }

                if (edge.loop.is_valid() && mesh.is_valid(edge.loop)) {
                    const geometry::Loop& loop = mesh.loop(edge.loop);
                    const geometry::EdgeHandle edgeHandle(static_cast<IdValue>(index));

                    if (loop.edge != edgeHandle) {
                        report.add_issue(
                            ValidationSeverity::Error,
                            "lem.reference.edge_loop_not_radial",
                            "ElementReferenceCheck",
                            "edge",
                            Id(static_cast<IdValue>(index)),
                            "Edge radial entry loop does not reference the edge."
                        );
                    }
                }
            }
        }

        static void validate_loops(const geometry::LEM& mesh, ValidationReport& report)
        {
            const auto& loops = mesh.loops();

            for (std::size_t index = 0; index < loops.size(); ++index) {
                const geometry::Loop& loop = loops[index];

                if (loop.deleted) {
                    continue;
                }

                const geometry::LoopHandle loopHandle(static_cast<IdValue>(index));

                if (mesh.is_valid(loop.face)) {
                    const geometry::Face& face = mesh.face(loop.face);

                    if (face.loop.is_invalid()) {
                        report.add_issue(
                            ValidationSeverity::Error,
                            "lem.reference.loop_face_without_entry",
                            "ElementReferenceCheck",
                            "loop",
                            loopHandle.id,
                            "Loop references a face without boundary entry loop."
                        );
                    }
                }

                if (!mesh.is_valid(loop.edge) || !mesh.is_valid(loop.vertex) || !mesh.is_valid(loop.next)) {
                    continue;
                }

                const geometry::Loop& nextLoop = mesh.loop(loop.next);

                if (!mesh.is_valid(nextLoop.vertex)) {
                    continue;
                }

                const geometry::Edge& edge = mesh.edge(loop.edge);
                const geometry::VertexHandle vertexA = loop.vertex;
                const geometry::VertexHandle vertexB = nextLoop.vertex;

                const bool matchesForward = edge.vertexA == vertexA && edge.vertexB == vertexB;
                const bool matchesBackward = edge.vertexA == vertexB && edge.vertexB == vertexA;

                if (!matchesForward && !matchesBackward) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.reference.loop_edge_vertices_mismatch",
                        "ElementReferenceCheck",
                        "loop",
                        loopHandle.id,
                        "Loop edge does not connect the loop vertex to the next loop vertex."
                    );
                }
            }
        }

        static void validate_faces(const geometry::LEM& mesh, ValidationReport& report)
        {
            const auto& faces = mesh.faces();

            for (std::size_t index = 0; index < faces.size(); ++index) {
                const geometry::Face& face = faces[index];

                if (face.deleted || face.loop.is_invalid() || !mesh.is_valid(face.loop)) {
                    continue;
                }

                const geometry::Loop& loop = mesh.loop(face.loop);
                const geometry::FaceHandle faceHandle(static_cast<IdValue>(index));

                if (loop.face != faceHandle) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.reference.face_loop_not_owned",
                        "ElementReferenceCheck",
                        "face",
                        Id(static_cast<IdValue>(index)),
                        "Face boundary entry loop is not owned by the face."
                    );
                }
            }
        }
    };

}