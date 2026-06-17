/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/RenderMesh.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/glm.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace locus::kernel::geometry
{
    /**
     * @brief Converts editable polygon faces into triangle render primitives.
     */
    class MeshTriangulator
    {
    public:
        /**
         * @brief Triangulates all active faces of a mesh into a new render mesh.
         *
         * @param mesh Source editable mesh.
         * @return Render mesh containing generated triangles.
         */
        [[nodiscard]] static RenderMesh triangulate(const LEM& mesh)
        {
            RenderMesh result;
            triangulate_into(mesh, result);
            return result;
        }

        /**
         * @brief Triangulates all active faces of a mesh into an existing output mesh.
         *
         * @param mesh Source editable mesh.
         * @param output Render mesh that receives generated triangles.
         */
        static void triangulate_into(const LEM& mesh, RenderMesh& output)
        {
            output.clear();

            for (FaceHandle faceHandle : TopologyTraversal::faces(mesh))
            {
                triangulate_face_into(mesh, faceHandle, output);
            }
        }

        /**
         * @brief Triangulates a single face into an existing output mesh.
         *
         * @param mesh Source editable mesh.
         * @param faceHandle Face to triangulate.
         * @param output Render mesh that receives generated triangles.
         * @note Invalid or degenerate faces are skipped when detected before emission.
         */
        static void triangulate_face_into(const LEM& mesh, FaceHandle faceHandle, RenderMesh& output)
        {
            if (!mesh.is_valid(faceHandle))
            {
                return;
            }

            const std::vector<LoopHandle> loops = mesh.face_loops(faceHandle);
            if (loops.size() < 3)
            {
                return;
            }

            const Face& face = mesh.face(faceHandle);
            std::vector<PolygonVertex> polygon;
            polygon.reserve(loops.size());

            for (LoopHandle loopHandle : loops)
            {
                if (!mesh.is_valid(loopHandle))
                {
                    return;
                }

                const Loop& loop = mesh.loop(loopHandle);
                if (!mesh.is_valid(loop.vertex))
                {
                    return;
                }

                const Vertex& vertex = mesh.vertex(loop.vertex);
                const RenderIndex renderIndex = output.add_vertex(vertex.position, face.normal);

                polygon.push_back(PolygonVertex{
                    loopHandle,
                    vertex.position,
                    project(vertex.position, face.normal),
                    renderIndex
                    });
            }

            if (polygon.size() == 3)
            {
                output.add_triangle(polygon[0].renderIndex, polygon[1].renderIndex, polygon[2].renderIndex);
                return;
            }

            if (!ear_clip(polygon, output))
            {
                triangulate_fan(polygon, output);
            }
        }

    private:
        /**
         * @brief Temporary polygon vertex used by the triangulation algorithm.
         */
        struct PolygonVertex
        {
            /**
             * @brief Source loop for this polygon corner.
             */
            LoopHandle loop{};

            /**
             * @brief Original 3D position.
             */
            glm::vec3 position{ 0.0f, 0.0f, 0.0f };

            /**
             * @brief 2D projection used for ear clipping.
             */
            glm::vec2 projected{ 0.0f, 0.0f };

            /**
             * @brief Render vertex index emitted for this polygon corner.
             */
            RenderIndex renderIndex = 0;
        };

        [[nodiscard]] static glm::vec2 project(const glm::vec3& point, const glm::vec3& normal)
        {
            const glm::vec3 absoluteNormal{
                std::abs(normal.x),
                std::abs(normal.y),
                std::abs(normal.z)
            };

            if (absoluteNormal.x >= absoluteNormal.y && absoluteNormal.x >= absoluteNormal.z)
            {
                return glm::vec2{ point.y, point.z };
            }

            if (absoluteNormal.y >= absoluteNormal.x && absoluteNormal.y >= absoluteNormal.z)
            {
                return glm::vec2{ point.x, point.z };
            }

            return glm::vec2{ point.x, point.y };
        }

        [[nodiscard]] static float signed_area(const std::vector<PolygonVertex>& polygon)
        {
            float area = 0.0f;

            for (std::size_t i = 0; i < polygon.size(); ++i)
            {
                const glm::vec2& current = polygon[i].projected;
                const glm::vec2& next = polygon[(i + 1) % polygon.size()].projected;
                area += current.x * next.y - next.x * current.y;
            }

            return area * 0.5f;
        }

        [[nodiscard]] static float cross_2d(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c)
        {
            const glm::vec2 ab = b - a;
            const glm::vec2 ac = c - a;
            return ab.x * ac.y - ab.y * ac.x;
        }

        [[nodiscard]] static bool is_convex(
            const glm::vec2& previous,
            const glm::vec2& current,
            const glm::vec2& next,
            float winding)
        {
            const float cross = cross_2d(previous, current, next);
            return winding >= 0.0f ? cross > 0.0f : cross < 0.0f;
        }

        [[nodiscard]] static bool point_in_triangle(
            const glm::vec2& point,
            const glm::vec2& a,
            const glm::vec2& b,
            const glm::vec2& c)
        {
            const float ab = cross_2d(a, b, point);
            const float bc = cross_2d(b, c, point);
            const float ca = cross_2d(c, a, point);

            const bool hasNegative = ab < 0.0f || bc < 0.0f || ca < 0.0f;
            const bool hasPositive = ab > 0.0f || bc > 0.0f || ca > 0.0f;

            return !(hasNegative && hasPositive);
        }

        [[nodiscard]] static bool is_ear(
            const std::vector<PolygonVertex>& polygon,
            const std::vector<std::size_t>& remaining,
            std::size_t previousIndex,
            std::size_t currentIndex,
            std::size_t nextIndex,
            float winding)
        {
            const PolygonVertex& previous = polygon[previousIndex];
            const PolygonVertex& current = polygon[currentIndex];
            const PolygonVertex& next = polygon[nextIndex];

            if (!is_convex(previous.projected, current.projected, next.projected, winding))
            {
                return false;
            }

            for (std::size_t index : remaining)
            {
                if (index == previousIndex || index == currentIndex || index == nextIndex)
                {
                    continue;
                }

                if (point_in_triangle(polygon[index].projected, previous.projected, current.projected, next.projected))
                {
                    return false;
                }
            }

            return true;
        }

        [[nodiscard]] static bool ear_clip(const std::vector<PolygonVertex>& polygon, RenderMesh& output)
        {
            if (polygon.size() < 3)
            {
                return false;
            }

            const float area = signed_area(polygon);
            if (area == 0.0f)
            {
                return false;
            }

            const float winding = area >= 0.0f ? 1.0f : -1.0f;

            std::vector<std::size_t> remaining;
            remaining.reserve(polygon.size());

            for (std::size_t i = 0; i < polygon.size(); ++i)
            {
                remaining.push_back(i);
            }

            std::size_t guard = 0;
            const std::size_t maxIterations = polygon.size() * polygon.size();

            while (remaining.size() > 3 && guard < maxIterations)
            {
                bool clipped = false;

                for (std::size_t i = 0; i < remaining.size(); ++i)
                {
                    const std::size_t previousIndex = remaining[(i + remaining.size() - 1) % remaining.size()];
                    const std::size_t currentIndex = remaining[i];
                    const std::size_t nextIndex = remaining[(i + 1) % remaining.size()];

                    if (!is_ear(polygon, remaining, previousIndex, currentIndex, nextIndex, winding))
                    {
                        continue;
                    }

                    output.add_triangle(
                        polygon[previousIndex].renderIndex,
                        polygon[currentIndex].renderIndex,
                        polygon[nextIndex].renderIndex
                    );

                    remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                    clipped = true;
                    break;
                }

                if (!clipped)
                {
                    return false;
                }

                ++guard;
            }

            if (remaining.size() != 3)
            {
                return false;
            }

            output.add_triangle(
                polygon[remaining[0]].renderIndex,
                polygon[remaining[1]].renderIndex,
                polygon[remaining[2]].renderIndex
            );

            return true;
        }

        static void triangulate_fan(const std::vector<PolygonVertex>& polygon, RenderMesh& output)
        {
            if (polygon.size() < 3)
            {
                return;
            }

            for (std::size_t i = 1; i + 1 < polygon.size(); ++i)
            {
                output.add_triangle(
                    polygon[0].renderIndex,
                    polygon[i].renderIndex,
                    polygon[i + 1].renderIndex
                );
            }
        }
    };
}
