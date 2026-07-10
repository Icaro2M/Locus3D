/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "GizmoTestSuite.h"

#include "common/TestLog.h"

#include <string>
#include <string_view>

namespace {

struct TestCase {
    std::string_view name;
    locus::tests::TestResult (*run)();
};

} // namespace

int main()
{
    using namespace locus::tests;

    const TestCase tests[] = {
        { "GizmoState", run_gizmo_state_tests },
        { "GizmoConstraint", run_gizmo_constraint_tests },
        { "GizmoSnap", run_gizmo_snap_tests },
        { "TransformGizmo", run_transform_gizmo_tests },
        { "GizmoController", run_gizmo_controller_tests },
    };

    for (const TestCase& test : tests) {
        const TestResult result = test.run();
        if (!result.success) {
            log_error(std::string(test.name) + ": " + result.message);
            return 1;
        }

        log_info(std::string(test.name) + ": passed");
    }

    return 0;
}
