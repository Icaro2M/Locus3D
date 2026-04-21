#pragma once

#include <vector>

#include "../../graphics/Shader.h"
#include "../../scene/Camera.h"
#include "FaceScaleTool.h"

class FaceScalePreviewRenderer
{
private:
    unsigned int m_VAO;
    unsigned int m_VBO;
    Shader m_Shader;

private:
    void buildFillVertices(
        const FaceScaleTool& tool,
        const Camera& camera,
        std::vector<glm::vec3>& outVertices
    ) const;

    void buildLineVertices(
        const FaceScaleTool& tool,
        const Camera& camera,
        std::vector<glm::vec3>& outVertices
    ) const;

public:
    FaceScalePreviewRenderer();
    ~FaceScalePreviewRenderer();

    void render(const FaceScaleTool& tool, const Camera& camera);
};