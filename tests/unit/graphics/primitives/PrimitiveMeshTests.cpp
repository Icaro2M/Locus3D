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

[[nodiscard]] locus::graphics::PrimitiveVertex vertex(float x, float y, float z)
{
    locus::graphics::PrimitiveVertex result;
    result.position = glm::vec3{ x, y, z };
    return result;
}

} // namespace

namespace locus::tests {

TestResult run_primitive_mesh_tests()
{
    using namespace graphics;

    PrimitiveVertex defaultVertex;
    if (!near_vec3(defaultVertex.position, glm::vec3{ 0.0f, 0.0f, 0.0f }) ||
        !near_vec3(defaultVertex.normal, glm::vec3{ 0.0f, 0.0f, 1.0f }) ||
        !near(defaultVertex.color.r, 1.0f) ||
        !near(defaultVertex.color.g, 1.0f) ||
        !near(defaultVertex.color.b, 1.0f) ||
        !near(defaultVertex.color.a, 1.0f)) {
        return TestResult::fail("PrimitiveVertex should default to origin, +Z normal, and white color");
    }

    PrimitiveMesh mesh;
    if (!mesh.is_empty() ||
        mesh.has_indices() ||
        mesh.element_count() != 0 ||
        mesh.is_valid()) {
        return TestResult::fail("empty PrimitiveMesh should report empty and invalid state");
    }

    mesh.vertices = {
        vertex(0.0f, 0.0f, 0.0f),
        vertex(1.0f, 0.0f, 0.0f),
        vertex(0.0f, 1.0f, 0.0f),
    };
    mesh.topology = PrimitiveTopology::Triangles;
    if (mesh.is_empty() ||
        mesh.has_indices() ||
        mesh.element_count() != 3 ||
        !mesh.is_valid()) {
        return TestResult::fail("non-indexed triangle mesh should be valid with three vertices");
    }

    mesh.indices = { 0, 1, 2 };
    if (!mesh.has_indices() ||
        mesh.element_count() != 3 ||
        !mesh.is_valid()) {
        return TestResult::fail("indexed triangle mesh should use index count as element count");
    }

    mesh.indices = { 0, 1, 3 };
    if (mesh.is_valid()) {
        return TestResult::fail("PrimitiveMesh should reject indices outside the vertex range");
    }

    mesh.indices.clear();
    mesh.topology = PrimitiveTopology::Lines;
    if (mesh.is_valid()) {
        return TestResult::fail("line mesh should reject odd element counts");
    }

    mesh.vertices.push_back(vertex(0.0f, 0.0f, 1.0f));
    if (!mesh.is_valid()) {
        return TestResult::fail("line mesh should accept an even element count of at least two");
    }

    mesh.topology = PrimitiveTopology::LineStrip;
    mesh.vertices.resize(2);
    if (!mesh.is_valid()) {
        return TestResult::fail("line strip mesh should accept at least two elements");
    }

    mesh.topology = PrimitiveTopology::TriangleStrip;
    if (mesh.is_valid()) {
        return TestResult::fail("triangle strip mesh should reject fewer than three elements");
    }

    mesh.vertices.push_back(vertex(0.0f, 0.0f, 1.0f));
    if (!mesh.is_valid()) {
        return TestResult::fail("triangle strip mesh should accept at least three elements");
    }

    mesh.topology = PrimitiveTopology::Points;
    mesh.vertices.resize(1);
    if (!mesh.is_valid()) {
        return TestResult::fail("point mesh should accept one or more vertices");
    }

    return TestResult::pass();
}

} // namespace locus::tests
