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
     * @brief Builds a torus as editable LEM topology.
     */
    class TorusBuilder {
    public:
        /**
         * @brief Creates a new mesh containing a torus primitive.
         *
         * @param parameters Torus creation parameters.
         * @return Mesh containing the torus, or an empty mesh for invalid parameters.
         */
        [[nodiscard]] static LEM build(const TorusParameters& parameters = {}) {
            LEM mesh;
            build_into(mesh, parameters);
            return mesh;
        }

        /**
         * @brief Appends a torus primitive to an existing mesh.
         *
         * @param mesh Mesh that receives the torus topology.
         * @param parameters Torus creation parameters.
         * @return Created element handles, recorded diff, and success state.
         */
        [[nodiscard]] static PrimitiveBuildResult build_into(
            LEM& mesh,
            const TorusParameters& parameters = {}) {
            PrimitiveBuildResult result;

            if (!parameters.is_valid()) {
                return result;
            }

            LEMEditor editor(mesh);
            const std::size_t edgeCount = mesh.edge_count();

            constexpr float pi = 3.14159265358979323846f;
            const float twoPi = pi * 2.0f;

            std::vector<std::vector<VertexHandle>> rings;
            rings.resize(parameters.majorSegments);

            result.vertices.reserve(parameters.majorSegments * parameters.minorSegments);
            result.faces.reserve(parameters.majorSegments * parameters.minorSegments);

            for (std::size_t major = 0; major < parameters.majorSegments; ++major) {
                const float u = twoPi * static_cast<float>(major) / static_cast<float>(parameters.majorSegments);
                const float cosU = std::cos(u);
                const float sinU = std::sin(u);

                std::vector<VertexHandle>& ring = rings[major];
                ring.reserve(parameters.minorSegments);

                for (std::size_t minor = 0; minor < parameters.minorSegments; ++minor) {
                    const float v = twoPi * static_cast<float>(minor) / static_cast<float>(parameters.minorSegments);
                    const float cosV = std::cos(v);
                    const float sinV = std::sin(v);

                    const float distance = parameters.majorRadius + parameters.minorRadius * cosV;

                    glm::vec3 position{
                        distance * cosU,
                        distance * sinU,
                        parameters.minorRadius * sinV
                    };

                    VertexHandle handle = editor.add_vertex(parameters.center + position);
                    ring.push_back(handle);
                    result.vertices.push_back(handle);
                }
            }

            for (std::size_t major = 0; major < parameters.majorSegments; ++major) {
                const std::size_t nextMajor = (major + 1) % parameters.majorSegments;

                for (std::size_t minor = 0; minor < parameters.minorSegments; ++minor) {
                    const std::size_t nextMinor = (minor + 1) % parameters.minorSegments;

                    FaceHandle faceHandle = editor.add_face({
                        rings[major][minor],
                        rings[nextMajor][minor],
                        rings[nextMajor][nextMinor],
                        rings[major][nextMinor]
                        });

                    if (!push_face(mesh, editor, result, faceHandle, parameters.selectCreatedFaces)) {
                        return fail(editor, result);
                    }
                }
            }

            collect_edges(mesh, result, edgeCount);

            result.diff = editor.take_diff();
            result.success = result.vertices.size() == parameters.majorSegments * parameters.minorSegments
                && result.faces.size() == parameters.majorSegments * parameters.minorSegments;

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
