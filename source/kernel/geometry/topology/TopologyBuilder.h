/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEM.h"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Mesh elements and diff produced by topology construction helpers.
     */
    struct TopologyBuildResult {
        /**
         * @brief Vertices created by the build.
         */
        std::vector<VertexHandle> vertices{};
        /**
         * @brief Edges created by the build.
         */
        std::vector<EdgeHandle> edges{};
        /**
         * @brief Faces created by the build.
         */
        std::vector<FaceHandle> faces{};
        /**
         * @brief Mesh changes recorded during the build.
         */
        LEMDiff diff{};
        /**
         * @brief True when the requested topology was fully created.
         */
        bool success = false;

        /**
         * @brief Converts the result to true when the build succeeded.
         */
        [[nodiscard]] explicit operator bool() const
        {
            return success;
        }

        /**
         * @brief Checks whether the build produced no mesh elements.
         *
         * @return True when all created element lists are empty.
         */
        [[nodiscard]] bool empty() const
        {
            return vertices.empty() && edges.empty() && faces.empty();
        }
    };

    /**
     * @brief Builds editable LEM topology from vertex positions and face indices.
     */
    class TopologyBuilder {
    public:
        /**
         * @brief Appends indexed polygon topology to an existing mesh.
         *
         * @param mesh Mesh that receives the new topology.
         * @param positions Vertex positions in object space.
         * @param faces Face vertex indices into positions.
         * @return Created element handles, recorded diff, and success state.
         */
        [[nodiscard]] static TopologyBuildResult build_into(
            LEM& mesh,
            const std::vector<glm::vec3>& positions,
            const std::vector<std::vector<std::size_t>>& faces
        )
        {
            TopologyBuildResult result;

            if (!has_valid_input(positions, faces)) {
                return result;
            }

            LEMEditor editor(mesh);
            const std::size_t firstEdge = mesh.edge_count();

            result.vertices.reserve(positions.size());
            result.faces.reserve(faces.size());

            for (const glm::vec3& position : positions) {
                VertexHandle vertexHandle = editor.add_vertex(position);
                if (!mesh.is_valid(vertexHandle)) {
                    result.diff = editor.take_diff();
                    result.success = false;
                    return result;
                }

                result.vertices.push_back(vertexHandle);
            }

            for (const std::vector<std::size_t>& faceIndices : faces) {
                std::vector<VertexHandle> faceVertices;
                faceVertices.reserve(faceIndices.size());

                for (std::size_t index : faceIndices) {
                    faceVertices.push_back(result.vertices[index]);
                }

                FaceHandle faceHandle = editor.add_face(faceVertices);
                if (!mesh.is_valid(faceHandle)) {
                    result.diff = editor.take_diff();
                    result.success = false;
                    return result;
                }

                result.faces.push_back(faceHandle);
            }

            for (std::size_t index = firstEdge; index < mesh.edge_count(); ++index) {
                EdgeHandle edgeHandle(static_cast<IdValue>(index));
                if (mesh.is_valid(edgeHandle)) {
                    result.edges.push_back(edgeHandle);
                }
            }

            result.diff = editor.take_diff();
            result.success = result.vertices.size() == positions.size()
                && result.faces.size() == faces.size();

            return result;
        }

        /**
         * @brief Creates a new mesh from indexed polygon topology.
         *
         * @param positions Vertex positions in object space.
         * @param faces Face vertex indices into positions.
         * @return Mesh containing the requested topology, or an empty mesh on invalid input.
         */
        [[nodiscard]] static LEM build(
            const std::vector<glm::vec3>& positions,
            const std::vector<std::vector<std::size_t>>& faces
        )
        {
            LEM mesh;
            build_into(mesh, positions, faces);
            return mesh;
        }

        /**
         * @brief Appends one quad face to an existing mesh.
         *
         * @param mesh Mesh that receives the quad.
         * @param a First quad vertex position.
         * @param b Second quad vertex position.
         * @param c Third quad vertex position.
         * @param d Fourth quad vertex position.
         * @return Created element handles, recorded diff, and success state.
         * @note The input order defines the quad winding and normal direction.
         */
        [[nodiscard]] static TopologyBuildResult build_quad_into(
            LEM& mesh,
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c,
            const glm::vec3& d
        )
        {
            return build_into(
                mesh,
                { a, b, c, d },
                { { 0, 1, 2, 3 } }
            );
        }

        /**
         * @brief Creates a new mesh containing one quad face.
         *
         * @param a First quad vertex position.
         * @param b Second quad vertex position.
         * @param c Third quad vertex position.
         * @param d Fourth quad vertex position.
         * @return Mesh containing the quad.
         * @note The input order defines the quad winding and normal direction.
         */
        [[nodiscard]] static LEM build_quad(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c,
            const glm::vec3& d
        )
        {
            LEM mesh;
            build_quad_into(mesh, a, b, c, d);
            return mesh;
        }

        /**
         * @brief Appends an axis-aligned box to an existing mesh.
         *
         * @param mesh Mesh that receives the box.
         * @param center Box center in object space.
         * @param size Box dimensions along the local X, Y, and Z axes.
         * @return Created element handles, recorded diff, and success state.
         */
        [[nodiscard]] static TopologyBuildResult build_box_into(
            LEM& mesh,
            const glm::vec3& center = glm::vec3{ 0.0f, 0.0f, 0.0f },
            const glm::vec3& size = glm::vec3{ 1.0f, 1.0f, 1.0f }
        )
        {
            if (size.x <= 0.0f || size.y <= 0.0f || size.z <= 0.0f) {
                return {};
            }

            const glm::vec3 half = size * 0.5f;
            const glm::vec3 min = center - half;
            const glm::vec3 max = center + half;

            return build_into(
                mesh,
                {
                    { min.x, min.y, min.z },
                    { max.x, min.y, min.z },
                    { max.x, max.y, min.z },
                    { min.x, max.y, min.z },
                    { min.x, min.y, max.z },
                    { max.x, min.y, max.z },
                    { max.x, max.y, max.z },
                    { min.x, max.y, max.z }
                },
            {
                { 0, 1, 5, 4 },
                { 3, 7, 6, 2 },
                { 4, 5, 6, 7 },
                { 0, 3, 2, 1 },
                { 1, 2, 6, 5 },
                { 0, 4, 7, 3 }
            }
            );
        }

        /**
         * @brief Creates a new mesh containing an axis-aligned box.
         *
         * @param center Box center in object space.
         * @param size Box dimensions along the local X, Y, and Z axes.
         * @return Mesh containing the box, or an empty mesh for invalid dimensions.
         */
        [[nodiscard]] static LEM build_box(
            const glm::vec3& center = glm::vec3{ 0.0f, 0.0f, 0.0f },
            const glm::vec3& size = glm::vec3{ 1.0f, 1.0f, 1.0f }
        )
        {
            LEM mesh;
            build_box_into(mesh, center, size);
            return mesh;
        }

    private:
        /**
         * @brief Validates source vertices and polygon index lists.
         *
         * @param positions Vertex positions in object space.
         * @param faces Face vertex indices into positions.
         * @return True when every face can be built safely.
         */
        [[nodiscard]] static bool has_valid_input(
            const std::vector<glm::vec3>& positions,
            const std::vector<std::vector<std::size_t>>& faces
        )
        {
            if (positions.empty() || faces.empty()) {
                return false;
            }

            for (const std::vector<std::size_t>& face : faces) {
                if (!has_valid_face_indices(positions.size(), face)) {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Validates one polygon's index list.
         *
         * @param vertexCount Number of available source vertices.
         * @param face Face index list.
         * @return True when the face has at least three unique in-range indices.
         */
        [[nodiscard]] static bool has_valid_face_indices(
            std::size_t vertexCount,
            const std::vector<std::size_t>& face
        )
        {
            if (face.size() < 3) {
                return false;
            }

            for (std::size_t i = 0; i < face.size(); ++i) {
                if (face[i] >= vertexCount) {
                    return false;
                }

                const std::size_t next = face[(i + 1) % face.size()];
                if (face[i] == next) {
                    return false;
                }

                for (std::size_t j = 0; j < i; ++j) {
                    if (face[i] == face[j]) {
                        return false;
                    }
                }
            }

            return true;
        }
    };

}
