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
#include <vector>

namespace locus::kernel::validation {

    /**
     * @brief Validates radial cycles around LEM edges.
     */
    class RadialCycleCheck final : public IValidationCheck {
    public:
        /**
         * @brief Returns the stable check name.
         *
         * @return Check name used in diagnostics.
         */
        [[nodiscard]] std::string_view name() const override
        {
            return "RadialCycleCheck";
        }

        /**
         * @brief Executes radial cycle validation.
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
            const auto& edges = mesh.edges();

            for (std::size_t index = 0; index < edges.size(); ++index) {
                const geometry::Edge& edge = edges[index];

                if (edge.deleted) {
                    continue;
                }

                validate_edge(mesh, geometry::EdgeHandle(static_cast<IdValue>(index)), report);
            }
        }

    private:
        static void validate_edge(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edgeHandle,
            ValidationReport& report)
        {
            const geometry::Edge& edge = mesh.edge(edgeHandle);

            if (edge.loop.is_invalid()) {
                return;
            }

            if (!mesh.is_valid(edge.loop)) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "lem.radial_cycle.invalid_entry_loop",
                    "RadialCycleCheck",
                    "edge",
                    edgeHandle.id,
                    "Edge radial entry loop is invalid."
                );
                return;
            }

            std::vector<bool> visited(mesh.loop_count(), false);

            geometry::LoopHandle current = edge.loop;
            std::size_t count = 0;

            while (true) {
                if (!mesh.is_valid(current)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.radial_cycle.invalid_loop",
                        "RadialCycleCheck",
                        "edge",
                        edgeHandle.id,
                        "Radial cycle reached an invalid loop."
                    );
                    return;
                }

                const IdValue currentIndex = current.id.value;

                if (currentIndex >= visited.size()) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.radial_cycle.out_of_range_loop",
                        "RadialCycleCheck",
                        "edge",
                        edgeHandle.id,
                        "Radial cycle reached an out-of-range loop."
                    );
                    return;
                }

                if (visited[currentIndex]) {
                    if (current == edge.loop) {
                        break;
                    }

                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.radial_cycle.repeated_loop",
                        "RadialCycleCheck",
                        "edge",
                        edgeHandle.id,
                        "Radial cycle reached a repeated loop before closing."
                    );
                    return;
                }

                visited[currentIndex] = true;
                ++count;

                const geometry::Loop& loop = mesh.loop(current);

                if (loop.edge != edgeHandle) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.radial_cycle.foreign_loop",
                        "RadialCycleCheck",
                        "loop",
                        current.id,
                        "Radial cycle contains a loop owned by another edge."
                    );
                }

                if (!mesh.is_valid(loop.radialNext) || !mesh.is_valid(loop.radialPrevious)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.radial_cycle.invalid_neighbor",
                        "RadialCycleCheck",
                        "loop",
                        current.id,
                        "Radial loop has an invalid radial next or previous link."
                    );
                    return;
                }

                const geometry::Loop& nextLoop = mesh.loop(loop.radialNext);
                const geometry::Loop& previousLoop = mesh.loop(loop.radialPrevious);

                if (nextLoop.radialPrevious != current) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.radial_cycle.next_previous_mismatch",
                        "RadialCycleCheck",
                        "loop",
                        current.id,
                        "Radial next link does not point back through radial previous."
                    );
                }

                if (previousLoop.radialNext != current) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.radial_cycle.previous_next_mismatch",
                        "RadialCycleCheck",
                        "loop",
                        current.id,
                        "Radial previous link does not point back through radial next."
                    );
                }

                current = loop.radialNext;

                if (count > mesh.loop_count()) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.radial_cycle.unbounded_cycle",
                        "RadialCycleCheck",
                        "edge",
                        edgeHandle.id,
                        "Radial traversal exceeded the mesh loop count."
                    );
                    return;
                }
            }
        }
    };

}