/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "EditorSceneTestSuite.h"

#include "editor/scene/NodeTransform.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <cmath>

namespace {

[[nodiscard]] bool almost_equal(float lhs, float rhs)
{
    return std::fabs(lhs - rhs) < 0.0001f;
}

[[nodiscard]] bool almost_equal(const glm::vec3& lhs, const glm::vec3& rhs)
{
    return almost_equal(lhs.x, rhs.x) &&
        almost_equal(lhs.y, rhs.y) &&
        almost_equal(lhs.z, rhs.z);
}

[[nodiscard]] bool almost_equal(const glm::quat& lhs, const glm::quat& rhs)
{
    return almost_equal(lhs.w, rhs.w) &&
        almost_equal(lhs.x, rhs.x) &&
        almost_equal(lhs.y, rhs.y) &&
        almost_equal(lhs.z, rhs.z);
}

} // namespace

namespace locus::tests {

TestResult run_node_transform_tests()
{
    editor::NodeTransform transform;

    if (!almost_equal(transform.position(), glm::vec3{ 0.0f, 0.0f, 0.0f })) {
        return TestResult::fail("default position should be zero");
    }

    if (!almost_equal(transform.scale(), glm::vec3{ 1.0f, 1.0f, 1.0f })) {
        return TestResult::fail("default scale should be one");
    }

    transform.set_position(glm::vec3{ 1.0f, 2.0f, 3.0f });
    transform.translate(glm::vec3{ 4.0f, 5.0f, 6.0f });
    transform.set_scale(glm::vec3{ 2.0f, 3.0f, 4.0f });
    transform.set_rotation(glm::quat{ 2.0f, 0.0f, 0.0f, 0.0f });

    if (!almost_equal(transform.position(), glm::vec3{ 5.0f, 7.0f, 9.0f })) {
        return TestResult::fail("translate should offset local position");
    }

    if (!almost_equal(transform.scale(), glm::vec3{ 2.0f, 3.0f, 4.0f })) {
        return TestResult::fail("set_scale should update local scale");
    }

    if (!almost_equal(transform.rotation(), glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f })) {
        return TestResult::fail("set_rotation should normalize quaternions");
    }

    const glm::mat4 matrix = transform.matrix();
    if (!almost_equal(matrix[3].x, 5.0f) ||
        !almost_equal(matrix[3].y, 7.0f) ||
        !almost_equal(matrix[3].z, 9.0f)) {
        return TestResult::fail("matrix should include local translation");
    }

    transform.reset();
    if (!almost_equal(transform.position(), glm::vec3{ 0.0f, 0.0f, 0.0f }) ||
        !almost_equal(transform.scale(), glm::vec3{ 1.0f, 1.0f, 1.0f }) ||
        !almost_equal(transform.rotation(), glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f })) {
        return TestResult::fail("reset should restore identity transform");
    }

    return TestResult::pass();
}

} // namespace locus::tests
