/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/primitives/PrimitiveParameters.h"

#include <glm/glm.hpp>

#include <cmath>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Builds a Z-axis cone as editable LEM topology.
     */
    class ConeBuilder {
    public:
        /**
         * @brief Creates a new mesh containing a cone primitive.
         *
         * @param parameters Cone creation parameters.
         * @return Mesh containing the cone, or an empty mesh for invalid parameters.
         */
        [[nodiscard]] static LEM build(const ConeParameters& parameters = {}) {
            LEM mesh;
            build_into(mesh, parameters);
            return mesh;
        }

        /**
         * @brief Appends a cone primitive to an existing mesh.
         *
         * @param mesh Mesh that receives the cone topology.
         * @param parameters Cone creation parameters.
         * @return Created element handles, recorded diff, and success state.
         */
        [[nodiscard]] static PrimitiveBuildResult build_into(
            LEM& mesh,
            const ConeParameters& parameters = {}) {
            PrimitiveBuildResult result;

            if (!parameters.is_valid()) {
                return result;
            }

            LEMEditor editor(mesh);
            const std::size_t edgeCount = mesh.edge_count();

            std::vector<VertexHandle> base;
            base.reserve(parameters.segments);

            const float halfHeight = parameters.height * 0.5f;
            constexpr float pi = 3.14159265358979323846f;
            const float twoPi = pi * 2.0f;

            for (std::size_t i = 0; i < parameters.segments; ++i) {
                const float angle = twoPi * static_cast<float>(i) / static_cast<float>(parameters.segments);
                const float x = std::cos(angle) * parameters.radius;
                const float y = std::sin(angle) * parameters.radius;

                VertexHandle handle = editor.add_vertex(parameters.center + glm::vec3{ x, y, -halfHeight });
                base.push_back(handle);
                result.vertices.push_back(handle);
            }

            VertexHandle apex = editor.add_vertex(parameters.center + glm::vec3{ 0.0f, 0.0f, halfHeight });
            result.vertices.push_back(apex);

            result.faces.reserve(parameters.segments + (parameters.capBottom ? 1 : 0));

            for (std::size_t i = 0; i < parameters.segments; ++i) {
                const std::size_t next = (i + 1) % parameters.segments;

                FaceHandle faceHandle = editor.add_face({
                    base[i],
                    base[next],
                    apex
                    });

                if (!push_face(mesh, editor, result, faceHandle, parameters.selectCreatedFaces)) {
                    return fail(editor, result);
                }
            }

            if (parameters.capBottom) {
                std::vector<VertexHandle> faceVertices;
                faceVertices.reserve(parameters.segments);

                for (std::size_t i = 0; i < parameters.segments; ++i) {
                    const std::size_t reversed = parameters.segments - 1 - i;
                    faceVertices.push_back(base[reversed]);
                }

                FaceHandle faceHandle = editor.add_face(faceVertices);

                if (!push_face(mesh, editor, result, faceHandle, parameters.selectCreatedFaces)) {
                    return fail(editor, result);
                }
            }

            collect_edges(mesh, result, edgeCount);

            result.diff = editor.take_diff();
            result.success = result.vertices.size() == parameters.segments + 1
                && result.faces.size() == parameters.segments + (parameters.capBottom ? 1 : 0);

            return result;
        }

    private:
        /**
         * @brief Adds a valid face to the build result and optionally selects it.
         *
         * @param mesh Mesh containing the face.
         * @param editor Editor used to record optional selection changes.
         * @param result Build result receiving the face handle.
         * @param faceHandle Face to append.
         * @param selected True when the face should be selected.
         * @return True when the face was valid and appended.
         */
        [[nodiscard]] static bool push_face(
            const LEM& mesh,
            LEMEditor& editor,
            PrimitiveBuildResult& result,
            FaceHandle faceHandle,
            bool selected) {
            if (!mesh.is_valid(faceHandle)) {
                return false;
            }

            if (selected) {
                editor.set_selected(faceHandle, true);
            }

            result.faces.push_back(faceHandle);
            return true;
        }

        /**
         * @brief Finalizes a failed build with the diff recorded so far.
         *
         * @param editor Editor that recorded partial changes.
         * @param result Partial build result.
         * @return Failed build result.
         */
        [[nodiscard]] static PrimitiveBuildResult fail(
            LEMEditor& editor,
            PrimitiveBuildResult& result) {
            result.diff = editor.take_diff();
            result.success = false;
            return result;
        }

        /**
         * @brief Collects edges created since the build started.
         *
         * @param mesh Mesh containing the created edges.
         * @param result Build result receiving edge handles.
         * @param edgeCount Edge count before the build started.
         */
        static void collect_edges(
            const LEM& mesh,
            PrimitiveBuildResult& result,
            std::size_t edgeCount) {
            for (std::size_t index = edgeCount; index < mesh.edge_count(); ++index) {
                EdgeHandle edgeHandle(static_cast<IdValue>(index));

                if (mesh.is_valid(edgeHandle)) {
                    result.edges.push_back(edgeHandle);
                }
            }
        }
    };

}
