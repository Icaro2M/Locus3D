/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "PrimitivesTestSuite.h"

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
        { "PrimitiveMesh", run_primitive_mesh_tests },
        { "PrimitiveBuilder", run_primitive_builder_tests },
        { "PrimitiveMeshConverter", run_primitive_mesh_converter_tests },
        { "ScreenSpaceLine", run_screen_space_line_tests },
        { "PointMarker", run_point_marker_tests },
        { "SurfaceOverlay", run_surface_overlay_tests },
        { "GizmoRendererGeometry", run_gizmo_renderer_geometry_tests },
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
