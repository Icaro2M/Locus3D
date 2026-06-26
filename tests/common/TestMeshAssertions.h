/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * @file TestMeshAssertions.h
 * @brief Future mesh-specific assertion helpers for tests.
 */

#include "TestResult.h"

#include <string>
#include <utility>

namespace locus::tests {

/**
 * @brief Converts a boolean condition into a TestResult.
 *
 * @param condition Condition to evaluate.
 * @param failure_message Message used when the condition is false.
 * @return Passing or failing result.
 */
[[nodiscard]] inline TestResult expect_true(bool condition, std::string failure_message)
{
    if (condition) {
        return TestResult::pass();
    }

    return TestResult::fail(std::move(failure_message));
}

/*
 * Future mesh assertions can be added here after the kernel mesh API is stable:
 *
 * - expect_vertex_count(mesh, count)
 * - expect_edge_count(mesh, count)
 * - expect_face_count(mesh, count)
 * - expect_mesh_valid(mesh)
 */

} // namespace locus::tests
