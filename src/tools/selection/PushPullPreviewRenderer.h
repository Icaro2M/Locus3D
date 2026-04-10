#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm/glm.hpp>

#include "../../graphics/Shader.h"
#include "../../scene/Camera.h"
#include "PushPullTool.h"

class PushPullPreviewRenderer
{
private:
    unsigned int m_VAO;
    unsigned int m_VBO;
    Shader m_Shader;

    void buildFillVertices(const PushPullTool& tool, std::vector<glm::vec3>& outVertices) const;
    void buildLineVertices(const PushPullTool& tool, std::vector<glm::vec3>& outVertices) const;

public:
    PushPullPreviewRenderer();
    ~PushPullPreviewRenderer();

    void render(const PushPullTool& tool, const Camera& camera);
};