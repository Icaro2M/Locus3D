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
)

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