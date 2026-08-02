/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "LEMTestSuite.h"

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
        { "LEMTopology", run_lem_topology_tests },
        { "LEMEditorDiff", run_lem_editor_diff_tests },
        { "LEMGeometry", run_lem_geometry_tests },
        { "LEMAttributes", run_lem_attribute_tests },
        { "BevelOperation", run_bevel_operation_tests },
        { "BridgeEdgeOperation", run_bridge_edge_operation_tests },
        { "FlipFaceOperation", run_flip_face_operation_tests },
        { "FillHoleOperation", run_fill_hole_operation_tests },
        { "DeleteMeshElementsOperation", run_delete_mesh_elements_operation_tests },
        { "DissolveMeshElementsOperation", run_dissolve_mesh_elements_operation_tests },
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
