/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/common/Id.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/validation/core/IValidationCheck.h"
#include "kernel/validation/core/ValidationSeverity.h"

#include <cmath>
#include <string>
#include <string_view>

namespace locus::kernel::validation {

    /**
     * @brief Validates whether editable mesh vertex positions contain finite values.
     */
    class InvalidPositionCheck final : public IValidationCheck {
    public:
        /**
         * @brief Returns the stable check name.
         *
         * @return Check name used in diagnostics.
         */
        [[nodiscard]] std::string_view name() const override
        {
            return "InvalidPositionCheck";
        }

        /**
         * @brief Executes invalid position validation.
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
            const auto& vertices = mesh.vertices();

            for (std::size_t index = 0; index < vertices.size(); ++index) {
                const geometry::Vertex& vertex = vertices[index];

                if (vertex.deleted) {
                    continue;
                }

                if (!is_finite(vertex.position.x) || !is_finite(vertex.position.y) || !is_finite(vertex.position.z)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "geometry.vertex.invalid_position",
                        std::string(name()),
                        "vertex",
                        Id(static_cast<IdValue>(index)),
                        "Vertex position contains NaN or infinite values."
                    );
                }
            }
        }

    private:
        [[nodiscard]] static bool is_finite(float value)
        {
            return std::isfinite(value);
        }
    };

}