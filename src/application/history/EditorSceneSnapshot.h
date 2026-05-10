#pragma once

#include "../../geometry/LogicalFace.h"
#include "../../tools/transform/TransformTypes.h"
#include "../EditorState.h"

#include <glm/glm/glm.hpp>

#include <string>
#include <vector>

struct EditorObjectSnapshot
{
    std::string name;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    std::vector<LogicalFace> logicalFaces;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);
};

struct EditorContextSnapshot
{
    int selectedObjectIndex = -1;
    bool faceModeActive = false;
    EditorToolType activeTool = EditorToolType::None;
    TransformMode transformMode = TransformMode::Translate;
    TransformSpace transformSpace = TransformSpace::Global;
};

struct EditorSceneSnapshot
{
    std::vector<EditorObjectSnapshot> objects;
    EditorContextSnapshot context;
};