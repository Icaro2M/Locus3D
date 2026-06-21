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
#include <string>
#include <string_view>
#include <vector>

namespace locus::kernel::validation {

    /**
     * @brief Validates global connectivity consistency in an editable mesh.
     */
    class ConnectivityConsistencyCheck final : public IValidationCheck {
    public:
        /**
         * @brief Returns the stable check name.
         *
         * @return Check name used in diagnostics.
         */
        [[nodiscard]] std::string_view name() const override
        {
            return "ConnectivityConsistencyCheck";
        }

        /**
         * @brief Executes connectivity consistency validation.
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

            ConnectivityState state{};
            state.vertexUsedByEdge.resize(mesh.vertex_count(), false);
            state.loopVisitedByFace.resize(mesh.loop_count(), false);
            state.loopVisitedByRadial.resize(mesh.loop_count(), false);

            validate_edges(mesh, state, report);
            validate_faces(mesh, state, report);
            validate_radial_cycles(mesh, state, report);
            validate_loops(mesh, state, report);
            validate_vertices(mesh, state, report);
        }

    private:
        /**
         * @brief Temporary reachability data collected while validating connectivity.
         */
        struct ConnectivityState {
            /**
             * @brief Marks active vertices referenced by at least one active edge.
             */
            std::vector<bool> vertexUsedByEdge{};

            /**
             * @brief Marks active loops reached through face boundary traversal.
             */
            std::vector<bool> loopVisitedByFace{};

            /**
             * @brief Marks active loops reached through edge radial traversal.
             */
            std::vector<bool> loopVisitedByRadial{};
        };

        /**
         * @brief Validates active edges and records vertex usage.
         *
         * @param mesh Editable mesh being validated.
         * @param state Connectivity state updated by this pass.
         * @param report Report that receives produced diagnostics.
         */
        static void validate_edges(
            const geometry::LEM& mesh,
            ConnectivityState& state,
            ValidationReport& report)
        {
            const auto& edges = mesh.edges();

            for (std::size_t index = 0; index < edges.size(); ++index) {
                const geometry::Edge& edge = edges[index];

                if (edge.deleted) {
                    continue;
                }

                const geometry::EdgeHandle edgeHandle(static_cast<IdValue>(index));

                if (!mesh.is_valid(edge.vertexA)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.edge_invalid_vertex_a",
                        "ConnectivityConsistencyCheck",
                        "edge",
                        edgeHandle.id,
                        "Active edge references an invalid first vertex."
                    );
                }
                else {
                    state.vertexUsedByEdge[edge.vertexA.id.value] = true;
                }

                if (!mesh.is_valid(edge.vertexB)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.edge_invalid_vertex_b",
                        "ConnectivityConsistencyCheck",
                        "edge",
                        edgeHandle.id,
                        "Active edge references an invalid second vertex."
                    );
                }
                else {
                    state.vertexUsedByEdge[edge.vertexB.id.value] = true;
                }

                if (edge.vertexA == edge.vertexB) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.edge_collapsed",
                        "ConnectivityConsistencyCheck",
                        "edge",
                        edgeHandle.id,
                        "Active edge connects the same vertex twice."
                    );
                }

                if (edge.loop.is_valid()) {
                    if (!mesh.is_valid(edge.loop)) {
                        report.add_issue(
                            ValidationSeverity::Error,
                            "topology.connectivity.edge_invalid_radial_entry",
                            "ConnectivityConsistencyCheck",
                            "edge",
                            edgeHandle.id,
                            "Active edge has an invalid radial entry loop."
                        );
                    }
                    else if (mesh.loop(edge.loop).edge != edgeHandle) {
                        report.add_issue(
                            ValidationSeverity::Error,
                            "topology.connectivity.edge_radial_entry_mismatch",
                            "ConnectivityConsistencyCheck",
                            "edge",
                            edgeHandle.id,
                            "Active edge radial entry loop does not point back to the edge."
                        );
                    }
                }
            }
        }

        /**
         * @brief Validates active faces and starts boundary reachability traversal.
         *
         * @param mesh Editable mesh being validated.
         * @param state Connectivity state updated by this pass.
         * @param report Report that receives produced diagnostics.
         */
        static void validate_faces(
            const geometry::LEM& mesh,
            ConnectivityState& state,
            ValidationReport& report)
        {
            const auto& faces = mesh.faces();

            for (std::size_t index = 0; index < faces.size(); ++index) {
                const geometry::Face& face = faces[index];

                if (face.deleted) {
                    continue;
                }

                const geometry::FaceHandle faceHandle(static_cast<IdValue>(index));

                if (!mesh.is_valid(face.loop)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.face_invalid_entry_loop",
                        "ConnectivityConsistencyCheck",
                        "face",
                        faceHandle.id,
                        "Active face has an invalid boundary entry loop."
                    );
                    continue;
                }

                traverse_face(mesh, faceHandle, state, report);
            }
        }

        /**
         * @brief Traverses a face boundary cycle and marks reached loops.
         *
         * @param mesh Editable mesh being validated.
         * @param faceHandle Face whose boundary cycle will be traversed.
         * @param state Connectivity state updated with reached loops.
         * @param report Report that receives produced diagnostics.
         */
        static void traverse_face(
            const geometry::LEM& mesh,
            geometry::FaceHandle faceHandle,
            ConnectivityState& state,
            ValidationReport& report)
        {
            const geometry::Face& face = mesh.face(faceHandle);
            geometry::LoopHandle current = face.loop;
            std::size_t traversed = 0;

            while (mesh.is_valid(current) && traversed <= mesh.loop_count()) {
                const IdValue loopIndex = current.id.value;

                if (state.loopVisitedByFace[loopIndex]) {
                    break;
                }

                const geometry::Loop& loop = mesh.loop(current);

                state.loopVisitedByFace[loopIndex] = true;
                ++traversed;

                if (loop.face != faceHandle) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.face_contains_foreign_loop",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        current.id,
                        "Face boundary traversal reached a loop owned by another face."
                    );
                }

                if (!mesh.is_valid(loop.next)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.face_invalid_next_loop",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        current.id,
                        "Face boundary traversal reached a loop with invalid next link."
                    );
                    return;
                }

                current = loop.next;

                if (current == face.loop) {
                    return;
                }
            }

            if (traversed > mesh.loop_count()) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "topology.connectivity.face_unbounded_cycle",
                    "ConnectivityConsistencyCheck",
                    "face",
                    faceHandle.id,
                    "Face boundary traversal exceeded the mesh loop count."
                );
            }
        }

        /**
         * @brief Validates active edge radial cycles and starts radial reachability traversal.
         *
         * @param mesh Editable mesh being validated.
         * @param state Connectivity state updated by this pass.
         * @param report Report that receives produced diagnostics.
         */
        static void validate_radial_cycles(
            const geometry::LEM& mesh,
            ConnectivityState& state,
            ValidationReport& report)
        {
            const auto& edges = mesh.edges();

            for (std::size_t index = 0; index < edges.size(); ++index) {
                const geometry::Edge& edge = edges[index];

                if (edge.deleted || edge.loop.is_invalid()) {
                    continue;
                }

                const geometry::EdgeHandle edgeHandle(static_cast<IdValue>(index));

                if (!mesh.is_valid(edge.loop)) {
                    continue;
                }

                traverse_radial(mesh, edgeHandle, state, report);
            }
        }

        /**
         * @brief Traverses an edge radial cycle and marks reached loops.
         *
         * @param mesh Editable mesh being validated.
         * @param edgeHandle Edge whose radial cycle will be traversed.
         * @param state Connectivity state updated with reached loops.
         * @param report Report that receives produced diagnostics.
         */
        static void traverse_radial(
            const geometry::LEM& mesh,
            geometry::EdgeHandle edgeHandle,
            ConnectivityState& state,
            ValidationReport& report)
        {
            const geometry::Edge& edge = mesh.edge(edgeHandle);
            geometry::LoopHandle current = edge.loop;
            std::size_t traversed = 0;

            while (mesh.is_valid(current) && traversed <= mesh.loop_count()) {
                const IdValue loopIndex = current.id.value;

                if (state.loopVisitedByRadial[loopIndex]) {
                    break;
                }

                const geometry::Loop& loop = mesh.loop(current);

                state.loopVisitedByRadial[loopIndex] = true;
                ++traversed;

                if (loop.edge != edgeHandle) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.radial_contains_foreign_loop",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        current.id,
                        "Radial traversal reached a loop owned by another edge."
                    );
                }

                if (!mesh.is_valid(loop.radialNext)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.radial_invalid_next_loop",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        current.id,
                        "Radial traversal reached a loop with invalid radial next link."
                    );
                    return;
                }

                current = loop.radialNext;

                if (current == edge.loop) {
                    return;
                }
            }

            if (traversed > mesh.loop_count()) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "topology.connectivity.radial_unbounded_cycle",
                    "ConnectivityConsistencyCheck",
                    "edge",
                    edgeHandle.id,
                    "Radial traversal exceeded the mesh loop count."
                );
            }
        }

        /**
         * @brief Validates active loops against face and edge reachability data.
         *
         * @param mesh Editable mesh being validated.
         * @param state Connectivity state produced by previous passes.
         * @param report Report that receives produced diagnostics.
         */
        static void validate_loops(
            const geometry::LEM& mesh,
            const ConnectivityState& state,
            ValidationReport& report)
        {
            const auto& loops = mesh.loops();

            for (std::size_t index = 0; index < loops.size(); ++index) {
                const geometry::Loop& loop = loops[index];

                if (loop.deleted) {
                    continue;
                }

                const geometry::LoopHandle loopHandle(static_cast<IdValue>(index));

                if (!mesh.is_valid(loop.face)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.loop_invalid_face",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        loopHandle.id,
                        "Active loop references an invalid face."
                    );
                }

                if (!mesh.is_valid(loop.edge)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.loop_invalid_edge",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        loopHandle.id,
                        "Active loop references an invalid edge."
                    );
                }

                if (!mesh.is_valid(loop.vertex)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.loop_invalid_vertex",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        loopHandle.id,
                        "Active loop references an invalid vertex."
                    );
                }

                if (loopHandle.id.value < state.loopVisitedByFace.size() && !state.loopVisitedByFace[loopHandle.id.value]) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.loop_not_reachable_from_face",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        loopHandle.id,
                        "Active loop is not reachable from its face boundary cycle."
                    );
                }

                if (loopHandle.id.value < state.loopVisitedByRadial.size() && !state.loopVisitedByRadial[loopHandle.id.value]) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.loop_not_reachable_from_edge",
                        "ConnectivityConsistencyCheck",
                        "loop",
                        loopHandle.id,
                        "Active loop is not reachable from its edge radial cycle."
                    );
                }

                validate_loop_edge_endpoint(mesh, loopHandle, report);
            }
        }

        /**
         * @brief Validates whether a loop edge connects the loop vertex to the next loop vertex.
         *
         * @param mesh Editable mesh being validated.
         * @param loopHandle Loop to validate.
         * @param report Report that receives produced diagnostics.
         */
        static void validate_loop_edge_endpoint(
            const geometry::LEM& mesh,
            geometry::LoopHandle loopHandle,
            ValidationReport& report)
        {
            const geometry::Loop& loop = mesh.loop(loopHandle);

            if (!mesh.is_valid(loop.edge) || !mesh.is_valid(loop.vertex) || !mesh.is_valid(loop.next)) {
                return;
            }

            const geometry::Loop& nextLoop = mesh.loop(loop.next);

            if (!mesh.is_valid(nextLoop.vertex)) {
                return;
            }

            const geometry::Edge& edge = mesh.edge(loop.edge);

            const bool forward = edge.vertexA == loop.vertex && edge.vertexB == nextLoop.vertex;
            const bool backward = edge.vertexB == loop.vertex && edge.vertexA == nextLoop.vertex;

            if (!forward && !backward) {
                report.add_issue(
                    ValidationSeverity::Error,
                    "topology.connectivity.loop_edge_endpoint_mismatch",
                    "ConnectivityConsistencyCheck",
                    "loop",
                    loopHandle.id,
                    "Loop edge does not connect the loop vertex to the next loop vertex."
                );
            }
        }

        /**
         * @brief Validates active vertices against edge usage and incident edge references.
         *
         * @param mesh Editable mesh being validated.
         * @param state Connectivity state produced by previous passes.
         * @param report Report that receives produced diagnostics.
         */
        static void validate_vertices(
            const geometry::LEM& mesh,
            const ConnectivityState& state,
            ValidationReport& report)
        {
            const auto& vertices = mesh.vertices();

            for (std::size_t index = 0; index < vertices.size(); ++index) {
                const geometry::Vertex& vertex = vertices[index];

                if (vertex.deleted) {
                    continue;
                }

                const geometry::VertexHandle vertexHandle(static_cast<IdValue>(index));

                if (vertex.edge.is_invalid()) {
                    continue;
                }

                if (!mesh.is_valid(vertex.edge)) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.vertex_invalid_incident_edge",
                        "ConnectivityConsistencyCheck",
                        "vertex",
                        vertexHandle.id,
                        "Active vertex references an invalid incident edge."
                    );
                    continue;
                }

                const geometry::Edge& edge = mesh.edge(vertex.edge);

                if (edge.vertexA != vertexHandle && edge.vertexB != vertexHandle) {
                    report.add_issue(
                        ValidationSeverity::Error,
                        "topology.connectivity.vertex_edge_not_incident",
                        "ConnectivityConsistencyCheck",
                        "vertex",
                        vertexHandle.id,
                        "Active vertex incident edge does not reference the vertex."
                    );
                }

                if (vertexHandle.id.value < state.vertexUsedByEdge.size() && !state.vertexUsedByEdge[vertexHandle.id.value]) {
                    report.add_issue(
                        ValidationSeverity::Warning,
                        "topology.connectivity.vertex_not_used_by_edge",
                        "ConnectivityConsistencyCheck",
                        "vertex",
                        vertexHandle.id,
                        "Active vertex is not referenced by any active edge."
                    );
                }
            }
        }
    };

}