# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

# CTest is enabled by the root CMakeLists.txt when LOCUS3D_BUILD_TESTS is ON.
# Calling enable_testing() here keeps this directory ready for direct reuse and
# is harmless when the parent project has already enabled testing.
enable_testing()

set(LOCUS3D_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LOCUS3D_TEST_DATA_DIR ${LOCUS3D_TESTS_DIR}/data)

add_executable(Locus3DKernelTests
    unit/kernel/geometry/mesh/LEMAttributeTests.cpp
    unit/kernel/geometry/mesh/LEMEditorDiffTests.cpp
    unit/kernel/geometry/mesh/LEMGeometryTests.cpp
    unit/kernel/geometry/mesh/LEMTestMain.cpp
    unit/kernel/geometry/mesh/LEMTopologyTests.cpp
)

target_include_directories(Locus3DKernelTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DKernelTests
    PRIVATE
        Locus3D::Geometry
)

add_test(NAME kernel.unit.geometry.mesh COMMAND Locus3DKernelTests)
set_tests_properties(kernel.unit.geometry.mesh PROPERTIES
    LABELS "unit;kernel;geometry;mesh"
)

add_executable(Locus3DEditorTests
    unit/editor/scene/EditorSceneTestMain.cpp
    unit/editor/scene/EditorSceneTests.cpp
    unit/editor/scene/MeshNodeTests.cpp
    unit/editor/scene/NodeTransformTests.cpp
    unit/editor/scene/SceneNodeTests.cpp
    unit/editor/scene/SceneTreeTests.cpp
)

target_include_directories(Locus3DEditorTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorTests
    PRIVATE
        Locus3D::Editor
)

add_test(NAME editor.unit.scene COMMAND Locus3DEditorTests)
set_tests_properties(editor.unit.scene PROPERTIES
    LABELS "unit;editor;scene"
)

add_executable(Locus3DEditorSelectionTests
    unit/editor/selection/MeshSelectionTests.cpp
    unit/editor/selection/ObjectSelectionTests.cpp
    unit/editor/selection/SelectionControllerTests.cpp
    unit/editor/selection/SelectionSetTests.cpp
    unit/editor/selection/SelectionStateTests.cpp
    unit/editor/selection/SelectionTestMain.cpp
)

target_include_directories(Locus3DEditorSelectionTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorSelectionTests
    PRIVATE
        Locus3D::Editor
)

add_test(NAME editor.unit.selection COMMAND Locus3DEditorSelectionTests)
set_tests_properties(editor.unit.selection PROPERTIES
    LABELS "unit;editor;selection"
)

add_executable(Locus3DEditorCommandTests
    unit/editor/command/CommandContextTests.cpp
    unit/editor/command/CommandDispatcherTests.cpp
    unit/editor/command/CommandRegistryTests.cpp
    unit/editor/command/CommandResultTests.cpp
    unit/editor/command/CommandTestMain.cpp
)

target_include_directories(Locus3DEditorCommandTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorCommandTests
    PRIVATE
        Locus3D::Editor
)

add_test(NAME editor.unit.command COMMAND Locus3DEditorCommandTests)
set_tests_properties(editor.unit.command PROPERTIES
    LABELS "unit;editor;command"
)

add_executable(Locus3DEditorHistoryTests
    unit/editor/history/HistoryEntryTests.cpp
    unit/editor/history/HistoryStackTests.cpp
    unit/editor/history/HistoryTestMain.cpp
)

target_include_directories(Locus3DEditorHistoryTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorHistoryTests
    PRIVATE
        Locus3D::Editor
)

add_test(NAME editor.unit.history COMMAND Locus3DEditorHistoryTests)
set_tests_properties(editor.unit.history PROPERTIES
    LABELS "unit;editor;history"
)

# Future test executables will be registered here as the manual test suites are
# added. Expected target groups include:
#
#   Locus3DGraphicsTests
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
