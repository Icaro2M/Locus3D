/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/glm.hpp>

#include <cmath>
#include <cstddef>
#include <vector>

namespace locus::kernel::geometry
{
    /**
     * @brief Selects how render mesh normals are generated.
     */
    enum class NormalBuildMode
    {
        /**
         * @brief Assigns each triangle normal directly to its vertices.
         */
        Flat,

        /**
         * @brief Averages normals across vertices that share the same position.
         */
        Smooth
    };

    /**
     * @brief Rebuilds face and render normals for editable and derived meshes.
     */
    class NormalBuilder
    {
    public:
        /**
         * @brief Computes the normal of a mesh face.
         *
         * @param mesh Source editable mesh.
         * @param faceHandle Face to evaluate.
         * @return Normal following the face winding, or the default up normal when invalid.
         */
        [[nodiscard]] static glm::vec3 face_normal(const LEM& mesh, FaceHandle faceHandle)
        {
            if (!mesh.is_valid(faceHandle))
            {
                return glm::vec3{ 0.0f, 1.0f, 0.0f };
            }

            const std::vector<VertexHandle> vertices = TopologyTraversal::face_vertices(mesh, faceHandle);
            return polygon_normal(mesh, vertices);
        }

        /**
         * @brief Recomputes stored normals for all active faces.
         *
         * @param mesh Mesh whose face normals will be updated.
         */
        static void rebuild_face_normals(LEM& mesh)
        {
            for (FaceHandle faceHandle : TopologyTraversal::faces(mesh))
            {
                mesh.face(faceHandle).normal = face_normal(mesh, faceHandle);
            }
        }

        /**
         * @brief Recomputes render mesh vertex normals.
         *
         * @param mesh Render mesh to update.
         * @param mode Normal generation mode.
         */
        static void rebuild_normals(RenderMesh& mesh, NormalBuildMode mode = NormalBuildMode::Flat)
        {
            if (mode == NormalBuildMode::Smooth)
            {
                rebuild_smooth_normals(mesh);
                return;
            }

            rebuild_flat_normals(mesh);
        }

        /**
         * @brief Recomputes normals using flat triangle shading.
         *
         * @param mesh Render mesh to update.
         */
        static void rebuild_flat_normals(RenderMesh& mesh)
        {
            for (RenderVertex& vertex : mesh.vertices)
            {
                vertex.normal = glm::vec3{ 0.0f, 0.0f, 0.0f };
            }

            for (const RenderTriangle& triangle : mesh.triangles)
            {
                if (!is_valid_triangle(mesh, triangle))
                {
                    continue;
                }

                const glm::vec3 normal = triangle_normal(
                    mesh.vertices[triangle.a].position,
                    mesh.vertices[triangle.b].position,
                    mesh.vertices[triangle.c].position
                );

                mesh.vertices[triangle.a].normal = normal;
                mesh.vertices[triangle.b].normal = normal;
                mesh.vertices[triangle.c].normal = normal;
            }
        }

        /**
         * @brief Recomputes normals by averaging coincident render vertices.
         *
         * @param mesh Render mesh to update.
         * @param epsilon Position matching tolerance.
         */
        static void rebuild_smooth_normals(RenderMesh& mesh, float epsilon = 1.0e-5f)
        {
            std::vector<glm::vec3> accumulated(mesh.vertices.size(), glm::vec3{ 0.0f, 0.0f, 0.0f });

            for (const RenderTriangle& triangle : mesh.triangles)
            {
                if (!is_valid_triangle(mesh, triangle))
                {
                    continue;
                }

                const glm::vec3 normal = triangle_normal(
                    mesh.vertices[triangle.a].position,
                    mesh.vertices[triangle.b].position,
                    mesh.vertices[triangle.c].position
                );

                accumulated[triangle.a] += normal;
                accumulated[triangle.b] += normal;
                accumulated[triangle.c] += normal;
            }

            for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
            {
                for (std::size_t j = i + 1; j < mesh.vertices.size(); ++j)
                {
                    if (same_position(mesh.vertices[i].position, mesh.vertices[j].position, epsilon))
                    {
                        const glm::vec3 combined = accumulated[i] + accumulated[j];
                        accumulated[i] = combined;
                        accumulated[j] = combined;
                    }
                }
            }

            for (std::size_t i = 0; i < mesh.vertices.size(); ++i)
            {
                mesh.vertices[i].normal = safe_normalize(accumulated[i]);
            }
        }

        /**
         * @brief Computes a normalized triangle normal.
         *
         * @param a First triangle vertex.
         * @param b Second triangle vertex.
         * @param c Third triangle vertex.
         * @return Unit normal following triangle winding, or the default up normal when degenerate.
         */
        [[nodiscard]] static glm::vec3 triangle_normal(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c)
        {
            return safe_normalize(glm::cross(b - a, c - a));
        }

        /**
         * @brief Computes a polygon normal using Newell's method.
         *
         * @param mesh Source editable mesh.
         * @param vertices Ordered polygon vertices.
         * @return Unit polygon normal, or the default up normal when invalid.
         */
        [[nodiscard]] static glm::vec3 polygon_normal(const LEM& mesh, const std::vector<VertexHandle>& vertices)
        {
            if (vertices.size() < 3)
            {
                return glm::vec3{ 0.0f, 1.0f, 0.0f };
            }

            glm::vec3 normal{ 0.0f, 0.0f, 0.0f };

            for (std::size_t i = 0; i < vertices.size(); ++i)
            {
                const VertexHandle currentHandle = vertices[i];
                const VertexHandle nextHandle = vertices[(i + 1) % vertices.size()];

                if (!mesh.is_valid(currentHandle) || !mesh.is_valid(nextHandle))
                {
                    return glm::vec3{ 0.0f, 1.0f, 0.0f };
                }

                const glm::vec3& current = mesh.vertex(currentHandle).position;
                const glm::vec3& next = mesh.vertex(nextHandle).position;

                normal.x += (current.y - next.y) * (current.z + next.z);
                normal.y += (current.z - next.z) * (current.x + next.x);
                normal.z += (current.x - next.x) * (current.y + next.y);
            }

            return safe_normalize(normal);
        }

    private:
        [[nodiscard]] static bool is_valid_triangle(const RenderMesh& mesh, const RenderTriangle& triangle)
        {
            return triangle.a < mesh.vertices.size()
                && triangle.b < mesh.vertices.size()
                && triangle.c < mesh.vertices.size()
                && triangle.a != triangle.b
                && triangle.b != triangle.c
                && triangle.c != triangle.a;
        }

        [[nodiscard]] static glm::vec3 safe_normalize(const glm::vec3& value)
        {
            const float length = glm::length(value);
            if (length <= 0.0f)
            {
                return glm::vec3{ 0.0f, 1.0f, 0.0f };
            }

            return value / length;
        }

        [[nodiscard]] static bool same_position(const glm::vec3& a, const glm::vec3& b, float epsilon)
        {
            return std::abs(a.x - b.x) <= epsilon
                && std::abs(a.y - b.y) <= epsilon
                && std::abs(a.z - b.z) <= epsilon;
        }
    };
}
