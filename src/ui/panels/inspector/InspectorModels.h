#pragma once

#include <string>
#include <vector>

#include <glm/glm/glm.hpp>

struct InspectorObjectItem
{
    int id = 0;
    std::string name;
    bool selected = false;
};

struct InspectorTransformData
{
    glm::vec3 position{ 0.0f };
    glm::vec3 rotation{ 0.0f };
    glm::vec3 scale{ 1.0f };
};

struct InspectorState
{
    std::vector<InspectorObjectItem> objects;
    int selectedObjectId = 0;
    std::string selectedObjectName;
    InspectorTransformData transform;
    bool hasSelection = false;
};