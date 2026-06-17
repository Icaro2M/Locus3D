/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace locus::kernel::geometry
{
    /**
     * @brief Index type used by render mesh primitives.
     */
    using RenderIndex = std::uint32_t;

    /**
     * @brief Vertex data prepared for rendering or preview generation.
     */
    struct RenderVertex
    {
        /**
         * @brief Vertex position.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Vertex normal used by shading.
         */
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
    };

    /**
     * @brief Indexed line primitive.
     */
    struct RenderLine
    {
        /**
         * @brief First vertex index.
         */
        RenderIndex a = 0;

        /**
         * @brief Second vertex index.
         */
        RenderIndex b = 0;
    };

    /**
     * @brief Indexed triangle primitive.
     */
    struct RenderTriangle
    {
        /**
         * @brief First vertex index.
         */
        RenderIndex a = 0;

        /**
         * @brief Second vertex index.
         */
        RenderIndex b = 0;

        /**
         * @brief Third vertex index.
         */
        RenderIndex c = 0;
    };

    /**
     * @brief Lightweight renderable mesh derived from editable geometry.
     */
    class RenderMesh
    {
    public:
        /**
         * @brief Checks whether the render mesh contains no primitives or vertices.
         *
         * @return True when vertices, triangles, and lines are all empty.
         */
        [[nodiscard]] bool empty() const
        {
            return vertices.empty()
                && triangles.empty()
                && lines.empty();
        }

        /**
         * @brief Returns the number of render vertices.
         *
         * @return Vertex count.
         */
        [[nodiscard]] std::size_t vertex_count() const
        {
            return vertices.size();
        }

        /**
         * @brief Returns the number of triangle primitives.
         *
         * @return Triangle count.
         */
        [[nodiscard]] std::size_t triangle_count() const
        {
            return triangles.size();
        }

        /**
         * @brief Returns the number of line primitives.
         *
         * @return Line count.
         */
        [[nodiscard]] std::size_t line_count() const
        {
            return lines.size();
        }

        /**
         * @brief Adds a render vertex.
         *
         * @param vertex Vertex data to append.
         * @return Index of the appended vertex.
         */
        [[nodiscard]] RenderIndex add_vertex(const RenderVertex& vertex)
        {
            const RenderIndex index = static_cast<RenderIndex>(vertices.size());
            vertices.push_back(vertex);
            return index;
        }

        /**
         * @brief Adds a render vertex with the default normal.
         *
         * @param position Vertex position.
         * @return Index of the appended vertex.
         */
        [[nodiscard]] RenderIndex add_vertex(const glm::vec3& position)
        {
            return add_vertex(RenderVertex{ position, glm::vec3{ 0.0f, 1.0f, 0.0f } });
        }

        /**
         * @brief Adds a render vertex with explicit position and normal.
         *
         * @param position Vertex position.
         * @param normal Vertex normal.
         * @return Index of the appended vertex.
         */
        [[nodiscard]] RenderIndex add_vertex(const glm::vec3& position, const glm::vec3& normal)
        {
            return add_vertex(RenderVertex{ position, normal });
        }

        /**
         * @brief Adds an indexed triangle primitive.
         *
         * @param a First vertex index.
         * @param b Second vertex index.
         * @param c Third vertex index.
         */
        void add_triangle(RenderIndex a, RenderIndex b, RenderIndex c)
        {
            triangles.push_back(RenderTriangle{ a, b, c });
        }

        /**
         * @brief Adds an indexed line primitive.
         *
         * @param a First vertex index.
         * @param b Second vertex index.
         */
        void add_line(RenderIndex a, RenderIndex b)
        {
            lines.push_back(RenderLine{ a, b });
        }

        /**
         * @brief Reserves storage for render vertices.
         *
         * @param count Target vertex capacity.
         */
        void reserve_vertices(std::size_t count)
        {
            vertices.reserve(count);
        }

        /**
         * @brief Reserves storage for triangle primitives.
         *
         * @param count Target triangle capacity.
         */
        void reserve_triangles(std::size_t count)
        {
            triangles.reserve(count);
        }

        /**
         * @brief Reserves storage for line primitives.
         *
         * @param count Target line capacity.
         */
        void reserve_lines(std::size_t count)
        {
            lines.reserve(count);
        }

        /**
         * @brief Removes all vertices and primitives.
         */
        void clear()
        {
            vertices.clear();
            triangles.clear();
            lines.clear();
        }

        /**
         * @brief Render vertex buffer.
         */
        std::vector<RenderVertex> vertices{};

        /**
         * @brief Indexed triangle buffer.
         */
        std::vector<RenderTriangle> triangles{};

        /**
         * @brief Indexed line buffer.
         */
        std::vector<RenderLine> lines{};
    };
}
