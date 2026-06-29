# SPDX-FileCopyrightText: 2026 Icaro2M
# SPDX-License-Identifier: Apache-2.0

add_library(locus3d_editor STATIC)
add_library(Locus3D::Editor ALIAS locus3d_editor)

target_sources(locus3d_editor
    PRIVATE
        "Editor.cpp"
        "Editor.h"
        "EditorContext.h"
        "EditorState.h"
        "EditorTypes.h"
        "scene/EditorScene.cpp"
        "scene/EditorScene.h"
        "scene/EmptyNode.h"
        "scene/MeshNode.h"
        "scene/NodeMetadata.h"
        "scene/NodePivot.h"
        "scene/NodeTransform.cpp"
        "scene/NodeTransform.h"
        "scene/NodeType.h"
        "scene/SceneNode.cpp"
        "scene/SceneNode.h"
        "scene/SceneNodeId.h"
        "scene/SceneTree.cpp"
        "scene/SceneTree.h"
        "selection/MeshSelection.cpp"
        "selection/SelectionGranularity.h"
        "selection/MeshSelection.h"
        "selection/ObjectSelection.cpp"
        "selection/ObjectSelection.h"
        "selection/SelectionController.cpp"
        "selection/SelectionController.h"
        "selection/SelectionGranularity.h"
        "selection/SelectionScope.h"
        "selection/SelectionSerializer.h"
        "selection/SelectionSet.cpp"
        "selection/SelectionSet.h"
        "selection/SelectionState.cpp"
        "selection/SelectionState.h"
        "command/CommandResult.h"
        "command/CommandContext.h"
        "command/ICommand.h"
        "command/CommandDispatcher.h"
        "command/CommandDispatcher.cpp"
        "command/CommandRegistry.h"
        "command/CommandRegistry.cpp")

target_include_directories(locus3d_editor
    PUBLIC
        ${LOCUS3D_SOURCE_DIR}
        ${GLM_DIR}/include
)

target_link_libraries(locus3d_editor
    PUBLIC
        Locus3D::Geometry
)

target_compile_definitions(locus3d_editor
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
)