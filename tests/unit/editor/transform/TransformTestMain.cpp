/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "TransformTestSuite.h"

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
        { "TransformTarget", run_transform_target_tests },
        { "TransformPivotResolver", run_transform_pivot_resolver_tests },
        { "TransformSession", run_transform_session_tests },
        { "MeshTransformTargetResolver", run_mesh_transform_target_resolver_tests },
        { "MeshTransformToolSession", run_mesh_transform_tool_session_tests },
        { "TransformToolSelection", run_transform_tool_selection_tests },
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
