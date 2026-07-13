/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/primitives/PrimitiveBuilder.h"

#include <utility>

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace locus::graphics {

    PrimitiveBuilder::PrimitiveBuilder(const PrimitiveTopology topology) {
        mesh_.topology = topology;
    }

    void PrimitiveBuilder::clear() {
        mesh_.vertices.clear();
        mesh_.indices.clear();
    }

    bool PrimitiveBuilder::add_point(
        const glm::vec3& position,
        const ColorRGBA& color
    ) {
        if (mesh_.topology != PrimitiveTopology::Points) {
            return false;
        }

        mesh_.vertices.push_back(
            make_vertex(position, glm::vec3{ 0.0f }, color)
        );

        return true;
    }

    bool PrimitiveBuilder::add_line(
        const glm::vec3& start,
        const glm::vec3& end,
        const ColorRGBA& color
    ) {
        return add_line(start, end, color, color);
    }

    bool PrimitiveBuilder::add_line(
        const glm::vec3& start,
        const glm::vec3& end,
        const ColorRGBA& startColor,
        const ColorRGBA& endColor
    ) {
        if (mesh_.topology != PrimitiveTopology::Lines) {
            return false;
        }

        const glm::vec3 normal{ 0.0f };

        mesh_.vertices.push_back(make_vertex(start, normal, startColor));
        mesh_.vertices.push_back(make_vertex(end, normal, endColor));

        return true;
    }

    bool PrimitiveBuilder::add_triangle(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const ColorRGBA& color
    ) {
        if (mesh_.topology != PrimitiveTopology::Triangles) {
            return false;
        }

        const glm::vec3 normal = triangle_normal(a, b, c);

        return add_triangle(
            make_vertex(a, normal, color),
            make_vertex(b, normal, color),
            make_vertex(c, normal, color)
        );
    }

    bool PrimitiveBuilder::add_triangle(
        const PrimitiveVertex& a,
        const PrimitiveVertex& b,
        const PrimitiveVertex& c
    ) {
        if (mesh_.topology != PrimitiveTopology::Triangles) {
            return false;
        }

        mesh_.vertices.push_back(a);
        mesh_.vertices.push_back(b);
        mesh_.vertices.push_back(c);

        return true;
    }

    bool PrimitiveBuilder::add_quad(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c,
        const glm::vec3& d,
        const ColorRGBA& color
    ) {
        if (mesh_.topology != PrimitiveTopology::Triangles) {
            return false;
        }

        add_triangle(a, b, c, color);
        add_triangle(a, c, d, color);

        return true;
    }

    bool PrimitiveBuilder::add_box_edges(
        const glm::vec3& minPoint,
        const glm::vec3& maxPoint,
        const ColorRGBA& color
    ) {
        if (mesh_.topology != PrimitiveTopology::Lines) {
            return false;
        }

        const glm::vec3 minimum = glm::min(minPoint, maxPoint);
        const glm::vec3 maximum = glm::max(minPoint, maxPoint);

        const glm::vec3 p000{
            minimum.x,
            minimum.y,
            minimum.z
        };

        const glm::vec3 p001{
            minimum.x,
            minimum.y,
            maximum.z
        };

        const glm::vec3 p010{
            minimum.x,
            maximum.y,
            minimum.z
        };

        const glm::vec3 p011{
            minimum.x,
            maximum.y,
            maximum.z
        };

        const glm::vec3 p100{
            maximum.x,
            minimum.y,
            minimum.z
        };

        const glm::vec3 p101{
            maximum.x,
            minimum.y,
            maximum.z
        };

        const glm::vec3 p110{
            maximum.x,
            maximum.y,
            minimum.z
        };

        const glm::vec3 p111{
            maximum.x,
            maximum.y,
            maximum.z
        };

        add_line(p000, p100, color);
        add_line(p100, p101, color);
        add_line(p101, p001, color);
        add_line(p001, p000, color);

        add_line(p010, p110, color);
        add_line(p110, p111, color);
        add_line(p111, p011, color);
        add_line(p011, p010, color);

        add_line(p000, p010, color);
        add_line(p100, p110, color);
        add_line(p101, p111, color);
        add_line(p001, p011, color);

        return true;
    }

    bool PrimitiveBuilder::is_empty() const {
        return mesh_.is_empty();
    }

    std::size_t PrimitiveBuilder::vertex_count() const {
        return mesh_.vertices.size();
    }

    PrimitiveTopology PrimitiveBuilder::topology() const {
        return mesh_.topology;
    }

    const PrimitiveMesh& PrimitiveBuilder::mesh() const {
        return mesh_;
    }

    PrimitiveMesh PrimitiveBuilder::build() {
        PrimitiveMesh result = std::move(mesh_);

        mesh_ = {};
        mesh_.topology = result.topology;

        return result;
    }

    PrimitiveVertex PrimitiveBuilder::make_vertex(
        const glm::vec3& position,
        const glm::vec3& normal,
        const ColorRGBA& color
    ) {
        PrimitiveVertex vertex;
        vertex.position = position;
        vertex.normal = normal;
        vertex.color = color;
        return vertex;
    }

    glm::vec3 PrimitiveBuilder::triangle_normal(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c
    ) {
        const glm::vec3 crossProduct = glm::cross(b - a, c - a);
        const float squaredLength = glm::dot(crossProduct, crossProduct);

        if (squaredLength <= 0.0f) {
            return glm::vec3{ 0.0f };
        }

        return crossProduct / glm::sqrt(squaredLength);
    }

} // namespace locus::graphics