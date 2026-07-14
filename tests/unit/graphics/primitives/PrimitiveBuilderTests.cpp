/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PrimitivesTestSuite.h"

#include "graphics/GraphicsPrimitives.h"

#include <cmath>

namespace {

constexpr float epsilon = 0.0001f;

[[nodiscard]] bool near(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] bool near_vec3(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

[[nodiscard]] bool near_color(
    const locus::graphics::ColorRGBA& lhs,
    const locus::graphics::ColorRGBA& rhs)
{
    return near(lhs.r, rhs.r) &&
        near(lhs.g, rhs.g) &&
        near(lhs.b, rhs.b) &&
        near(lhs.a, rhs.a);
}

} // namespace

namespace locus::tests {

TestResult run_primitive_builder_tests()
{
    using namespace graphics;

    const ColorRGBA red{ 1.0f, 0.0f, 0.0f, 1.0f };
    const ColorRGBA green{ 0.0f, 1.0f, 0.0f, 1.0f };
    const ColorRGBA blue{ 0.0f, 0.0f, 1.0f, 1.0f };

    PrimitiveBuilder points(PrimitiveTopology::Points);
    if (!points.is_empty() ||
        points.vertex_count() != 0 ||
        points.topology() != PrimitiveTopology::Points ||
        !points.add_point(glm::vec3{ 1.0f, 2.0f, 3.0f }, red) ||
        points.add_line(glm::vec3{ 0.0f }, glm::vec3{ 1.0f }, green) ||
        points.vertex_count() != 1) {
        return TestResult::fail("point builder should accept points and reject line operations");
    }

    const PrimitiveMesh pointMesh = points.build();
    if (!pointMesh.is_valid() ||
        pointMesh.topology != PrimitiveTopology::Points ||
        pointMesh.vertices.size() != 1 ||
        !near_vec3(pointMesh.vertices[0].position, glm::vec3{ 1.0f, 2.0f, 3.0f }) ||
        !near_vec3(pointMesh.vertices[0].normal, glm::vec3{ 0.0f, 0.0f, 0.0f }) ||
        !near_color(pointMesh.vertices[0].color, red) ||
        !points.is_empty() ||
        points.topology() != PrimitiveTopology::Points) {
        return TestResult::fail("build should move point geometry out and preserve builder topology");
    }

    PrimitiveBuilder lines(PrimitiveTopology::Lines);
    if (!lines.add_line(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 0.0f, 0.0f },
            red,
            green) ||
        lines.add_point(glm::vec3{ 0.0f }, blue) ||
        lines.vertex_count() != 2) {
        return TestResult::fail("line builder should accept lines and reject point operations");
    }

    const PrimitiveMesh& linePreview = lines.mesh();
    if (!near_color(linePreview.vertices[0].color, red) ||
        !near_color(linePreview.vertices[1].color, green) ||
        !near_vec3(linePreview.vertices[0].normal, glm::vec3{ 0.0f })) {
        return TestResult::fail("line builder should preserve per-endpoint colors and zero normals");
    }

    if (!lines.add_box_edges(
            glm::vec3{ 1.0f, 2.0f, 3.0f },
            glm::vec3{ -1.0f, -2.0f, -3.0f },
            blue) ||
        lines.vertex_count() != 26 ||
        !near_vec3(lines.mesh().vertices[2].position, glm::vec3{ -1.0f, -2.0f, -3.0f }) ||
        !near_vec3(lines.mesh().vertices[3].position, glm::vec3{ 1.0f, -2.0f, -3.0f })) {
        return TestResult::fail("add_box_edges should normalize inverted bounds and append twelve edges");
    }

    lines.clear();
    if (!lines.is_empty() ||
        lines.topology() != PrimitiveTopology::Lines) {
        return TestResult::fail("clear should remove vertices while preserving line topology");
    }

    PrimitiveBuilder triangles(PrimitiveTopology::Triangles);
    if (!triangles.add_triangle(
            glm::vec3{ 0.0f, 0.0f, 0.0f },
            glm::vec3{ 1.0f, 0.0f, 0.0f },
            glm::vec3{ 0.0f, 1.0f, 0.0f },
            green) ||
        triangles.add_line(glm::vec3{ 0.0f }, glm::vec3{ 1.0f }, red) ||
        triangles.vertex_count() != 3) {
        return TestResult::fail("triangle builder should accept triangles and reject line operations");
    }

    if (!near_vec3(triangles.mesh().vertices[0].normal, glm::vec3{ 0.0f, 0.0f, 1.0f }) ||
        !near_color(triangles.mesh().vertices[2].color, green)) {
        return TestResult::fail("add_triangle should generate a normalized face normal and copy color");
    }

    if (!triangles.add_quad(
            glm::vec3{ 0.0f, 0.0f, 1.0f },
            glm::vec3{ 1.0f, 0.0f, 1.0f },
            glm::vec3{ 1.0f, 1.0f, 1.0f },
            glm::vec3{ 0.0f, 1.0f, 1.0f },
            blue) ||
        triangles.vertex_count() != 9) {
        return TestResult::fail("add_quad should append two triangles");
    }

    PrimitiveVertex a;
    PrimitiveVertex b;
    PrimitiveVertex c;
    a.position = glm::vec3{ 0.0f };
    b.position = glm::vec3{ 1.0f, 0.0f, 0.0f };
    c.position = glm::vec3{ 2.0f, 0.0f, 0.0f };
    if (!triangles.add_triangle(a.position, b.position, c.position, red) ||
        !near_vec3(triangles.mesh().vertices[9].normal, glm::vec3{ 0.0f })) {
        return TestResult::fail("degenerate triangle should receive a zero normal");
    }

    a.color = red;
    b.color = green;
    c.color = blue;
    a.normal = glm::vec3{ 1.0f, 0.0f, 0.0f };
    b.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
    c.normal = glm::vec3{ 0.0f, 0.0f, 1.0f };
    if (!triangles.add_triangle(a, b, c) ||
        !near_color(triangles.mesh().vertices[12].color, red) ||
        !near_vec3(triangles.mesh().vertices[13].normal, glm::vec3{ 0.0f, 1.0f, 0.0f })) {
        return TestResult::fail("explicit triangle overload should preserve per-vertex attributes");
    }

    return TestResult::pass();
}

} // namespace locus::tests
