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
     * @brief Validates whether stored LEM handles point to active elements.
     */
    class HandleValidityCheck final : public IValidationCheck {
    public:
        /**
         * @brief Returns the stable check name.
         *
         * @return Check name used in diagnostics.
         */
        [[nodiscard]] std::string_view name() const override
        {
            return "HandleValidityCheck";
        }

        /**
         * @brief Executes handle validity validation.
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

                if (vertex.deleted) {
                    continue;
                }

                if (vertex.edge.is_valid() && !mesh.is_valid(vertex.edge)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.vertex.invalid_edge",
                        "HandleValidityCheck",
                        "vertex",
                        Id(static_cast<IdValue>(index)),
                        "Vertex references an invalid incident edge."
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

                if (!mesh.is_valid(edge.vertexA)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.edge.invalid_vertex_a",
                        "HandleValidityCheck",
                        "edge",
                        Id(static_cast<IdValue>(index)),
                        "Edge references an invalid first vertex."
                    );
                }

                if (!mesh.is_valid(edge.vertexB)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.edge.invalid_vertex_b",
                        "HandleValidityCheck",
                        "edge",
                        Id(static_cast<IdValue>(index)),
                        "Edge references an invalid second vertex."
                    );
                }

                if (edge.loop.is_valid() && !mesh.is_valid(edge.loop)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.edge.invalid_loop",
                        "HandleValidityCheck",
                        "edge",
                        Id(static_cast<IdValue>(index)),
                        "Edge references an invalid radial entry loop."
                    );
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

                if (!mesh.is_valid(loop.vertex)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.loop.invalid_vertex",
                        "HandleValidityCheck",
                        "loop",
                        Id(static_cast<IdValue>(index)),
                        "Loop references an invalid vertex."
                    );
                }

                if (!mesh.is_valid(loop.edge)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.loop.invalid_edge",
                        "HandleValidityCheck",
                        "loop",
                        Id(static_cast<IdValue>(index)),
                        "Loop references an invalid edge."
                    );
                }

                if (!mesh.is_valid(loop.face)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.loop.invalid_face",
                        "HandleValidityCheck",
                        "loop",
                        Id(static_cast<IdValue>(index)),
                        "Loop references an invalid face."
                    );
                }

                if (!mesh.is_valid(loop.next)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.loop.invalid_next",
                        "HandleValidityCheck",
                        "loop",
                        Id(static_cast<IdValue>(index)),
                        "Loop references an invalid next loop."
                    );
                }

                if (!mesh.is_valid(loop.previous)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.loop.invalid_previous",
                        "HandleValidityCheck",
                        "loop",
                        Id(static_cast<IdValue>(index)),
                        "Loop references an invalid previous loop."
                    );
                }

                if (!mesh.is_valid(loop.radialNext)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.loop.invalid_radial_next",
                        "HandleValidityCheck",
                        "loop",
                        Id(static_cast<IdValue>(index)),
                        "Loop references an invalid radial next loop."
                    );
                }

                if (!mesh.is_valid(loop.radialPrevious)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.loop.invalid_radial_previous",
                        "HandleValidityCheck",
                        "loop",
                        Id(static_cast<IdValue>(index)),
                        "Loop references an invalid radial previous loop."
                    );
                }
            }
        }

        static void validate_faces(const geometry::LEM& mesh, ValidationReport& report)
        {
            const auto& faces = mesh.faces();

            for (std::size_t index = 0; index < faces.size(); ++index) {
                const geometry::Face& face = faces[index];

                if (face.deleted) {
                    continue;
                }

                if (!mesh.is_valid(face.loop)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face.invalid_loop",
                        "HandleValidityCheck",
                        "face",
                        Id(static_cast<IdValue>(index)),
                        "Face references an invalid boundary loop."
                    );
                }
            }
        }
    };

}