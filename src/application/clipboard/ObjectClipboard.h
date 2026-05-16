#pragma once

#include "../controllers/SceneContext.h"
#include "../../scene/SceneObject.h"
#include "../../geometry/MeshFaceData.h"

#include <glm/glm/glm.hpp>

#include <string>
#include <vector>

class ObjectClipboard {
public:
    void copyFrom(const SceneObject& object);
    SceneObject* pasteInto(SceneContext& sceneContext);

    bool hasData() const;
    void clear();

private:
    std::string m_name;
    std::vector<float> m_vertices;
    std::vector<unsigned int> m_indices;
    std::vector<LogicalFace> m_logicalFaces;

    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f);
    glm::vec3 m_scale = glm::vec3(1.0f);

    bool m_hasData = false;
    int m_pasteCount = 0;
};