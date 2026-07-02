/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "CommandCommandsTestSuite.h"

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
        { "CreateNodeCommands", run_create_node_command_tests },
        { "NodeMetadataCommands", run_node_metadata_command_tests },
        { "NodeHierarchyCommands", run_node_hierarchy_command_tests },
        { "NodeTransformCommands", run_node_transform_command_tests },
        { "ObjectSelectionCommands", run_object_selection_command_tests },
        { "SelectionModeCommands", run_selection_mode_command_tests },
        { "MeshSelectionCommands", run_mesh_selection_command_tests },
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
