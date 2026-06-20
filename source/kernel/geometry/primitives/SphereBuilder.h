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
     * @brief Builds a UV sphere as editable LEM topology.
     */
    class SphereBuilder {
    public:
        /**
         * @brief Creates a new mesh containing a sphere primitive.
         *
         * @param parameters Sphere creation parameters.
         * @return Mesh containing the sphere, or an empty mesh for invalid parameters.
         */
        [[nodiscard]] static LEM build(const SphereParameters& parameters = {}) {
            LEM mesh;
            build_into(mesh, parameters);
            return mesh;
        }

        /**
         * @brief Appends a UV sphere primitive to an existing mesh.
         *
         * @param mesh Mesh that receives the sphere topology.
         * @param parameters Sphere creation parameters.
         * @return Created element handles, recorded diff, and success state.
         */
        [[nodiscard]] static PrimitiveBuildResult build_into(
            LEM& mesh,
            const SphereParameters& parameters = {}) {
            PrimitiveBuildResult result;

            if (!parameters.is_valid()) {
                return result;
            }

            LEMEditor editor(mesh);
            const std::size_t edgeCount = mesh.edge_count();

            constexpr float pi = 3.14159265358979323846f;
            const float twoPi = pi * 2.0f;

            VertexHandle top = editor.add_vertex(parameters.center + glm::vec3{ 0.0f, 0.0f, parameters.radius });
            VertexHandle bottom = editor.add_vertex(parameters.center + glm::vec3{ 0.0f, 0.0f, -parameters.radius });

            result.vertices.push_back(top);

            const std::size_t ringCount = parameters.latitudeSegments - 1;
            std::vector<std::vector<VertexHandle>> rings;
            rings.resize(ringCount);

            for (std::size_t latitude = 1; latitude < parameters.latitudeSegments; ++latitude) {
                const float v = pi * static_cast<float>(latitude) / static_cast<float>(parameters.latitudeSegments);
                const float z = std::cos(v) * parameters.radius;
                const float ringRadius = std::sin(v) * parameters.radius;

                std::vector<VertexHandle>& ring = rings[latitude - 1];
                ring.reserve(parameters.longitudeSegments);

                for (std::size_t longitude = 0; longitude < parameters.longitudeSegments; ++longitude) {
                    const float u = twoPi * static_cast<float>(longitude) / static_cast<float>(parameters.longitudeSegments);
                    const float x = std::cos(u) * ringRadius;
                    const float y = std::sin(u) * ringRadius;

                    VertexHandle handle = editor.add_vertex(parameters.center + glm::vec3{ x, y, z });
                    ring.push_back(handle);
                    result.vertices.push_back(handle);
                }
            }

            result.vertices.push_back(bottom);

            const std::size_t expectedFaces = parameters.longitudeSegments * parameters.latitudeSegments;
            result.faces.reserve(expectedFaces);

            const std::vector<VertexHandle>& firstRing = rings.front();

            for (std::size_t i = 0; i < parameters.longitudeSegments; ++i) {
                const std::size_t next = (i + 1) % parameters.longitudeSegments;

                FaceHandle faceHandle = editor.add_face({
                    top,
                    firstRing[i],
                    firstRing[next]
                    });

                if (!push_face(mesh, editor, result, faceHandle, parameters.selectCreatedFaces)) {
                    return fail(editor, result);
                }
            }

            for (std::size_t ringIndex = 0; ringIndex + 1 < rings.size(); ++ringIndex) {
                const std::vector<VertexHandle>& currentRing = rings[ringIndex];
                const std::vector<VertexHandle>& nextRing = rings[ringIndex + 1];

                for (std::size_t i = 0; i < parameters.longitudeSegments; ++i) {
                    const std::size_t next = (i + 1) % parameters.longitudeSegments;

                    FaceHandle faceHandle = editor.add_face({
                        currentRing[i],
                        nextRing[i],
                        nextRing[next],
                        currentRing[next]
                        });

                    if (!push_face(mesh, editor, result, faceHandle, parameters.selectCreatedFaces)) {
                        return fail(editor, result);
                    }
                }
            }

            const std::vector<VertexHandle>& lastRing = rings.back();

            for (std::size_t i = 0; i < parameters.longitudeSegments; ++i) {
                const std::size_t next = (i + 1) % parameters.longitudeSegments;

                FaceHandle faceHandle = editor.add_face({
                    lastRing[i],
                    bottom,
                    lastRing[next]
                    });

                if (!push_face(mesh, editor, result, faceHandle, parameters.selectCreatedFaces)) {
                    return fail(editor, result);
                }
            }

            collect_edges(mesh, result, edgeCount);

            result.diff = editor.take_diff();
            result.success = result.vertices.size() == 2 + ringCount * parameters.longitudeSegments
                && result.faces.size() == expectedFaces;

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
