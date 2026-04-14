#pragma once

#include <vector>

#include <glad/glad.h>
#include <glm/glm/glm.hpp>

#include "../../graphics/Shader.h"
#include "../../scene/Camera.h"
#include "FaceMoveTool.h"

class FaceMovePreviewRenderer
{
private:
    unsigned int m_VAO;
    unsigned int m_VBO;
    Shader m_Shader;

    void buildFillVertices(const FaceMoveTool& tool, std::vector<glm::vec3>& outVertices) const;
    void buildLineVertices(const FaceMoveTool& tool, std::vector<glm::vec3>& outVertices) const;

public:
    FaceMovePreviewRenderer();
    ~FaceMovePreviewRenderer();

    void render(const FaceMoveTool& tool, const Camera& camera);
};