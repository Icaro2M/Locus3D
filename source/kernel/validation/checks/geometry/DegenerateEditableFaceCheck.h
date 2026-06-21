/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Id.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/math/GeometryMath.h"
#include "kernel/validation/core/IValidationCheck.h"
#include "kernel/validation/core/ValidationSeverity.h"

#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace locus::kernel::validation {

    /**
     * @brief Validates degenerate editable polygon faces.
     */
    class DegenerateEditableFaceCheck final : public IValidationCheck {
    public:
        /**
         * @brief Returns the stable check name.
         *
         * @return Check name used in diagnostics.
         */
        [[nodiscard]] std::string_view name() const override
        {
            return "DegenerateEditableFaceCheck";
        }

        /**
         * @brief Executes editable face degeneracy validation.
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
            const auto& faces = mesh.faces();

            for (std::size_t index = 0; index < faces.size(); ++index) {
                const geometry::Face& face = faces[index];

                if (face.deleted) {
                    continue;
                }

                validate_face(mesh, geometry::FaceHandle(static_cast<IdValue>(index)), report);
            }
        }

    private:
        static void validate_face(
            const geometry::LEM& mesh,
            geometry::FaceHandle faceHandle,
            ValidationReport& report)
        {
            if (!mesh.is_valid(faceHandle)) {
                return;
            }

            const std::vector<geometry::LoopHandle> loops = mesh.face_loops(faceHandle);

            if (loops.size() < 3) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "geometry.face.too_few_corners",
                    "DegenerateEditableFaceCheck",
                    "face",
                    faceHandle.id,
                    "Editable face has fewer than three boundary loops."
                );
                return;
            }

            std::vector<geometry::VertexHandle> vertices{};
            vertices.reserve(loops.size());

            for (geometry::LoopHandle loopHandle : loops) {
                if (!mesh.is_valid(loopHandle)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "geometry.face.invalid_loop",
                        "DegenerateEditableFaceCheck",
                        "face",
                        faceHandle.id,
                        "Editable face references an invalid loop while checking degeneracy."
                    );
                    return;
                }

                const geometry::Loop& loop = mesh.loop(loopHandle);

                if (!mesh.is_valid(loop.vertex)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "geometry.face.invalid_vertex",
                        "DegenerateEditableFaceCheck",
                        "face",
                        faceHandle.id,
                        "Editable face references an invalid vertex while checking degeneracy."
                    );
                    return;
                }

                vertices.push_back(loop.vertex);
            }

            validate_repeated_vertices(vertices, faceHandle, report);
            validate_repeated_positions(mesh, vertices, faceHandle, report);
            validate_area(mesh, vertices, faceHandle, report);
        }

        static void validate_repeated_vertices(
            const std::vector<geometry::VertexHandle>& vertices,
            geometry::FaceHandle faceHandle,
            ValidationReport& report)
        {
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                for (std::size_t j = i + 1; j < vertices.size(); ++j) {
                    if (vertices[i] == vertices[j]) {
                        report.add_issue(
                            ValidationSeverity::Error,
                            "geometry.face.repeated_vertex",
                            "DegenerateEditableFaceCheck",
                            "face",
                            faceHandle.id,
                            "Editable face contains the same vertex handle more than once."
                        );
                        return;
                    }
                }
            }
        }

        static void validate_repeated_positions(
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& vertices,
            geometry::FaceHandle faceHandle,
            ValidationReport& report)
        {
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                const glm::vec3& a = mesh.vertex(vertices[i]).position;

                for (std::size_t j = i + 1; j < vertices.size(); ++j) {
                    const glm::vec3& b = mesh.vertex(vertices[j]).position;

                    if (glm::length(a - b) <= math::Epsilon) {
                        report.add_issue(
                            ValidationSeverity::Error,
                            "geometry.face.repeated_position",
                            "DegenerateEditableFaceCheck",
                            "face",
                            faceHandle.id,
                            "Editable face contains two vertices with nearly identical positions."
                        );
                        return;
                    }
                }
            }
        }

        static void validate_area(
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& vertices,
            geometry::FaceHandle faceHandle,
            ValidationReport& report)
        {
            const float area = polygon_area(mesh, vertices);

            if (!std::isfinite(area) || area <= math::Epsilon) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "geometry.face.degenerate_area",
                    "DegenerateEditableFaceCheck",
                    "face",
                    faceHandle.id,
                    "Editable face has zero or near-zero area."
                );
            }
        }

        [[nodiscard]] static float polygon_area(
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& vertices)
        {
            if (vertices.size() < 3) {
                return 0.0f;
            }

            const glm::vec3& origin = mesh.vertex(vertices[0]).position;
            float area = 0.0f;

            for (std::size_t i = 1; i + 1 < vertices.size(); ++i) {
                const glm::vec3& a = mesh.vertex(vertices[i]).position;
                const glm::vec3& b = mesh.vertex(vertices[i + 1]).position;

                area += math::triangle_area(origin, a, b);
            }

            return area;
        }
    };

}