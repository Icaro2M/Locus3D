# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

# CTest is enabled by the root CMakeLists.txt when LOCUS3D_BUILD_TESTS is ON.
# Calling enable_testing() here keeps this directory ready for direct reuse and
# is harmless when the parent project has already enabled testing.
enable_testing()

set(LOCUS3D_TESTS_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(LOCUS3D_TEST_DATA_DIR ${LOCUS3D_TESTS_DIR}/data)

add_executable(Locus3DGeometryTests
    unit/kernel/geometry/mesh/LEMAttributeTests.cpp
    unit/kernel/geometry/mesh/LEMEditorDiffTests.cpp
    unit/kernel/geometry/mesh/LEMGeometryTests.cpp
    unit/kernel/geometry/mesh/LEMTestMain.cpp
    unit/kernel/geometry/mesh/LEMTopologyTests.cpp
)

target_include_directories(Locus3DGeometryTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DGeometryTests
    PRIVATE
        Locus3D::Geometry
)

add_test(NAME kernel.unit.geometry.mesh COMMAND Locus3DGeometryTests)
set_tests_properties(kernel.unit.geometry.mesh PROPERTIES
    LABELS "unit;kernel;geometry;mesh"
)

add_executable(Locus3DSpatialTests
    unit/kernel/geometry/spatial/BVHQueryTests.cpp
    unit/kernel/geometry/spatial/BVHTests.cpp
    unit/kernel/geometry/spatial/SpatialIndexTests.cpp
    unit/kernel/geometry/spatial/SpatialTestMain.cpp
)

target_include_directories(Locus3DSpatialTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DSpatialTests
    PRIVATE
        Locus3D::Geometry
)

add_test(NAME kernel.unit.geometry.spatial COMMAND Locus3DSpatialTests)
set_tests_properties(kernel.unit.geometry.spatial PROPERTIES
    LABELS "unit;kernel;geometry;spatial"
)

add_executable(Locus3DGeometryQueryTests
    unit/kernel/geometry/queries/AdjacencyQueryTests.cpp
    unit/kernel/geometry/queries/BoundsQueryTests.cpp
    unit/kernel/geometry/queries/PickingQueryTests.cpp
    unit/kernel/geometry/queries/QueryTestMain.cpp
    unit/kernel/geometry/queries/SelectionHitTests.cpp
)

target_include_directories(Locus3DGeometryQueryTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DGeometryQueryTests
    PRIVATE
        Locus3D::Geometry
)

add_test(NAME kernel.unit.geometry.queries COMMAND Locus3DGeometryQueryTests)
set_tests_properties(kernel.unit.geometry.queries PROPERTIES
    LABELS "unit;kernel;geometry;queries"
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

add_executable(Locus3DEditorRenderTests
    ${GLAD_DIR}/src/glad.c
    unit/editor/render/EditorRenderTestMain.cpp
    unit/editor/render/MeshNodeRenderAdapterTests.cpp
    unit/editor/render/RenderMeshUploadAdapterTests.cpp
    unit/editor/render/SceneRenderAdapterTests.cpp
    unit/editor/render/SelectionRenderAdapterTests.cpp
)

target_include_directories(Locus3DEditorRenderTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorRenderTests
    PRIVATE
        Locus3D::Editor
        Locus3D::Graphics
)

add_test(NAME editor.unit.render COMMAND Locus3DEditorRenderTests)
set_tests_properties(editor.unit.render PROPERTIES
    LABELS "unit;editor;render"
)

add_executable(Locus3DEditorTransformTests
    unit/editor/transform/TransformPivotResolverTests.cpp
    unit/editor/transform/TransformSessionTests.cpp
    unit/editor/transform/TransformTargetTests.cpp
    unit/editor/transform/TransformTestMain.cpp
)

target_include_directories(Locus3DEditorTransformTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorTransformTests
    PRIVATE
        Locus3D::Editor
)

add_test(NAME editor.unit.transform COMMAND Locus3DEditorTransformTests)
set_tests_properties(editor.unit.transform PROPERTIES
    LABELS "unit;editor;transform"
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

add_executable(Locus3DEditorCommandCommandsTests
    unit/editor/command/commands/CommandCommandsTestMain.cpp
    unit/editor/command/commands/scene/CreateNodeCommandTests.cpp
    unit/editor/command/commands/scene/NodeHierarchyCommandTests.cpp
    unit/editor/command/commands/scene/NodeMetadataCommandTests.cpp
    unit/editor/command/commands/selection/MeshSelectionCommandTests.cpp
    unit/editor/command/commands/selection/ObjectSelectionCommandTests.cpp
    unit/editor/command/commands/selection/SelectionModeCommandTests.cpp
    unit/editor/command/commands/transform/NodeTransformCommandTests.cpp
)

target_include_directories(Locus3DEditorCommandCommandsTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorCommandCommandsTests
    PRIVATE
        Locus3D::Editor
)

add_test(NAME editor.unit.command.commands COMMAND Locus3DEditorCommandCommandsTests)
set_tests_properties(editor.unit.command.commands PROPERTIES
    LABELS "unit;editor;command;commands"
)

add_executable(Locus3DEditorCommandMeshTests
    unit/editor/command/mesh/ApplyMeshOperationCommandTests.cpp
    unit/editor/command/mesh/EditMeshSelectionCommandTests.cpp
    unit/editor/command/mesh/MeshCommandTestMain.cpp
    unit/editor/command/mesh/MeshSnapshotTests.cpp
    unit/editor/command/mesh/ReplaceMeshCommandTests.cpp
)

target_include_directories(Locus3DEditorCommandMeshTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorCommandMeshTests
    PRIVATE
        Locus3D::Editor
)

add_test(NAME editor.unit.command.mesh COMMAND Locus3DEditorCommandMeshTests)
set_tests_properties(editor.unit.command.mesh PROPERTIES
    LABELS "unit;editor;command;mesh"
)

add_executable(Locus3DEditorCommandDocumentTests
    unit/editor/command/document/ClearSceneCommandTests.cpp
    unit/editor/command/document/DocumentCommandTestMain.cpp
    unit/editor/command/document/ImportMeshCommandTests.cpp
)

target_include_directories(Locus3DEditorCommandDocumentTests
    PRIVATE
        ${LOCUS3D_SOURCE_DIR}
        ${LOCUS3D_TESTS_DIR}
        ${LOCUS3D_TESTS_DIR}/common
)

target_link_libraries(Locus3DEditorCommandDocumentTests
    PRIVATE
        Locus3D::Editor
)

add_test(NAME editor.unit.command.document COMMAND Locus3DEditorCommandDocumentTests)
set_tests_properties(editor.unit.command.document PROPERTIES
    LABELS "unit;editor;command;document"
)

add_executable(Locus3DEditorHistoryTests
    unit/editor/history/HistoryConfigTests.cpp
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
