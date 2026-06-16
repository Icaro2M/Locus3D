#pragma once

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace locus::kernel::geometry
{
    using RenderIndex = std::uint32_t;

    struct RenderVertex
    {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
    };

    struct RenderLine
    {
        RenderIndex a = 0;
        RenderIndex b = 0;
    };

    struct RenderTriangle
    {
        RenderIndex a = 0;
        RenderIndex b = 0;
        RenderIndex c = 0;
    };

    class RenderMesh
    {
    public:
        [[nodiscard]] bool empty() const
        {
            return vertices.empty()
                && triangles.empty()
                && lines.empty();
        }

        [[nodiscard]] std::size_t vertex_count() const
        {
            return vertices.size();
        }

        [[nodiscard]] std::size_t triangle_count() const
        {
            return triangles.size();
        }

        [[nodiscard]] std::size_t line_count() const
        {
            return lines.size();
        }

        [[nodiscard]] RenderIndex add_vertex(const RenderVertex& vertex)
        {
            const RenderIndex index = static_cast<RenderIndex>(vertices.size());
            vertices.push_back(vertex);
            return index;
        }

        [[nodiscard]] RenderIndex add_vertex(const glm::vec3& position)
        {
            return add_vertex(RenderVertex{ position, glm::vec3{ 0.0f, 1.0f, 0.0f } });
        }

        [[nodiscard]] RenderIndex add_vertex(const glm::vec3& position, const glm::vec3& normal)
        {
            return add_vertex(RenderVertex{ position, normal });
        }

        void add_triangle(RenderIndex a, RenderIndex b, RenderIndex c)
        {
            triangles.push_back(RenderTriangle{ a, b, c });
        }

        void add_line(RenderIndex a, RenderIndex b)
        {
            lines.push_back(RenderLine{ a, b });
        }

        void reserve_vertices(std::size_t count)
        {
            vertices.reserve(count);
        }

        void reserve_triangles(std::size_t count)
        {
            triangles.reserve(count);
        }

        void reserve_lines(std::size_t count)
        {
            lines.reserve(count);
        }

        void clear()
        {
            vertices.clear();
            triangles.clear();
            lines.clear();
        }

        std::vector<RenderVertex> vertices{};
        std::vector<RenderTriangle> triangles{};
        std::vector<RenderLine> lines{};
    };
}