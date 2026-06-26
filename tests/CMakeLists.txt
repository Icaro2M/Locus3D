# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

# CTest is enabled by the root CMakeLists.txt when LOCUS3D_BUILD_TESTS is ON.
# Calling enable_testing() here keeps this directory ready for direct reuse and
# is harmless when the parent project has already enabled testing.
enable_testing()

set(LOCUS3D_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LOCUS3D_TEST_DATA_DIR ${LOCUS3D_TESTS_DIR}/data)

# Future test executables will be registered here as the manual test suites are
# added. Expected target groups include:
#
#   Locus3DKernelTests
#   Locus3DGraphicsTests
#   Locus3DEditorTests
#   Locus3DIntegrationTests
#   Locus3DVisualTests
#
# Example structure for future tests:
#
#   add_executable(Locus3DKernelTests
#       unit/kernel/SomeKernelTest.cpp
#   )
#
#   target_include_directories(Locus3DKernelTests PRIVATE
#       ${LOCUS3D_SOURCE_DIR}
#       ${LOCUS3D_TESTS_DIR}/common
#   )
#
#   target_link_libraries(Locus3DKernelTests PRIVATE
#       Locus3D::Geometry
#   )
#
#   add_test(NAME kernel.unit.mesh COMMAND Locus3DKernelTests)
#   set_tests_properties(kernel.unit.mesh PROPERTIES
#       LABELS "unit;kernel"
#   )
#
# Other future label examples:
#
#   set_tests_properties(graphics.unit.renderer PROPERTIES
#       LABELS "unit;graphics"
#   )
#
#   set_tests_properties(editor.unit.tools PROPERTIES
#       LABELS "unit;editor"
#   )
#
#   set_tests_properties(editor_kernel.integration.selection PROPERTIES
#       LABELS "integration;editor;kernel"
#   )
#
#   set_tests_properties(import_export.regression.obj PROPERTIES
#       LABELS "regression;kernel"
#   )
#
#   set_tests_properties(graphics.visual.viewport PROPERTIES
#       LABELS "visual;graphics"
#   )
