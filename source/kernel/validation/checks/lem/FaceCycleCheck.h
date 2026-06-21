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
     * @brief Validates boundary cycles around LEM faces.
     */
    class FaceCycleCheck final : public IValidationCheck {
    public:
        /**
         * @brief Returns the stable check name.
         *
         * @return Check name used in diagnostics.
         */
        [[nodiscard]] std::string_view name() const override
        {
            return "FaceCycleCheck";
        }

        /**
         * @brief Executes face cycle validation.
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
            const geometry::Face& face = mesh.face(faceHandle);

            if (!mesh.is_valid(face.loop)) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "lem.face_cycle.invalid_entry_loop",
                    "FaceCycleCheck",
                    "face",
                    faceHandle.id,
                    "Face boundary entry loop is invalid."
                );
                return;
            }

            std::vector<bool> visited(mesh.loop_count(), false);

            geometry::LoopHandle current = face.loop;
            std::size_t count = 0;

            while (true) {
                if (!mesh.is_valid(current)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face_cycle.invalid_loop",
                        "FaceCycleCheck",
                        "face",
                        faceHandle.id,
                        "Face boundary reached an invalid loop."
                    );
                    return;
                }

                const IdValue currentIndex = current.id.value;

                if (currentIndex >= visited.size()) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face_cycle.out_of_range_loop",
                        "FaceCycleCheck",
                        "face",
                        faceHandle.id,
                        "Face boundary reached an out-of-range loop."
                    );
                    return;
                }

                if (visited[currentIndex]) {
                    if (current == face.loop) {
                        break;
                    }

                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face_cycle.repeated_loop",
                        "FaceCycleCheck",
                        "face",
                        faceHandle.id,
                        "Face boundary reached a repeated loop before closing."
                    );
                    return;
                }

                visited[currentIndex] = true;
                ++count;

                const geometry::Loop& loop = mesh.loop(current);

                if (loop.face != faceHandle) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face_cycle.foreign_loop",
                        "FaceCycleCheck",
                        "loop",
                        current.id,
                        "Face boundary contains a loop owned by another face."
                    );
                }

                if (!mesh.is_valid(loop.next) || !mesh.is_valid(loop.previous)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face_cycle.invalid_neighbor",
                        "FaceCycleCheck",
                        "loop",
                        current.id,
                        "Face boundary loop has an invalid next or previous link."
                    );
                    return;
                }

                const geometry::Loop& nextLoop = mesh.loop(loop.next);
                const geometry::Loop& previousLoop = mesh.loop(loop.previous);

                if (nextLoop.previous != current) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face_cycle.next_previous_mismatch",
                        "FaceCycleCheck",
                        "loop",
                        current.id,
                        "Loop next link does not point back through previous."
                    );
                }

                if (previousLoop.next != current) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face_cycle.previous_next_mismatch",
                        "FaceCycleCheck",
                        "loop",
                        current.id,
                        "Loop previous link does not point back through next."
                    );
                }

                current = loop.next;

                if (count > mesh.loop_count()) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "lem.face_cycle.unbounded_cycle",
                        "FaceCycleCheck",
                        "face",
                        faceHandle.id,
                        "Face boundary traversal exceeded the mesh loop count."
                    );
                    return;
                }
            }

            if (count < 3) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "lem.face_cycle.too_small",
                    "FaceCycleCheck",
                    "face",
                    faceHandle.id,
                    "Face boundary contains fewer than three loops."
                );
            }
        }
    };

}