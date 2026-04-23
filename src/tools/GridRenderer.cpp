#include "GridRenderer.h"

#include "../resources/AssetPaths.h"

#include <glad/glad.h>

#include <glm/glm/glm.hpp>
#include <vector>
#include <cmath>

namespace
{
    void appendLine(
        std::vector<float>& vertices,
        const glm::vec3& start,
        const glm::vec3& end,
        const glm::vec3& color,
        float lineStrength
    )
    {
        vertices.push_back(start.x);
        vertices.push_back(start.y);
        vertices.push_back(start.z);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(lineStrength);

        vertices.push_back(end.x);
        vertices.push_back(end.y);
        vertices.push_back(end.z);
        vertices.push_back(color.r);
        vertices.push_back(color.g);
        vertices.push_back(color.b);
        vertices.push_back(lineStrength);
    }
}

GridRenderer::GridRenderer()
    : m_VBO(nullptr),
    m_Shader(
        AssetPaths::shader("helpers/grid/gridVertex.glsl"),
        AssetPaths::shader("helpers/grid/gridFragment.glsl")
    ),
    m_VertexCount(0)
{
    const float gridExtent = 200.0f;
    const float lineSpacing = 5.0f;
    const int gridDivisions = static_cast<int>(gridExtent / lineSpacing);
    const int majorLineStep = 5;

    const glm::vec3 minorColor(0.34f, 0.34f, 0.34f);
    const glm::vec3 majorColor(0.48f, 0.48f, 0.48f);

    std::vector<float> vertices;
    vertices.reserve((gridDivisions * 4 + 4) * 14);

    for (int i = -gridDivisions; i <= gridDivisions; i++)
    {
        if (i == 0)
        {
            continue;
        }

        const float lineOffset = static_cast<float>(i) * lineSpacing;
        const bool isMajorLine = (std::abs(i) % majorLineStep) == 0;

        const glm::vec3 lineColor = isMajorLine ? majorColor : minorColor;
        const float lineStrength = isMajorLine ? 1.0f : 0.72f;

        appendLine(
            vertices,
            glm::vec3(-gridExtent, 0.0f, lineOffset),
            glm::vec3(gridExtent, 0.0f, lineOffset),
            lineColor,
            lineStrength
        );

        appendLine(
            vertices,
            glm::vec3(lineOffset, 0.0f, -gridExtent),
            glm::vec3(lineOffset, 0.0f, gridExtent),
            lineColor,
            lineStrength
        );
    }

    m_VertexCount = static_cast<unsigned int>(vertices.size() / 7);

    m_VAO.bind();

    m_VBO = new VertexBuffer(
        vertices.data(),
        static_cast<unsigned int>(vertices.size() * sizeof(float))
    );
    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

GridRenderer::~GridRenderer()
{
    delete m_VBO;
}

void GridRenderer::render(const Camera& camera)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());
    m_Shader.setMat4("u_Model", glm::mat4(1.0f));

    m_Shader.setFloat("u_FadeStart", 40.0f);
    m_Shader.setFloat("u_FadeEnd", 90.0f);
    m_Shader.setFloat("u_MinAlpha", 0.0f);  
    m_Shader.setFloat("u_MaxAlpha", 0.85f);

    m_VAO.bind();
    glDrawArrays(GL_LINES, 0, m_VertexCount);

    glDepthMask(GL_TRUE);  
    glDisable(GL_BLEND);
}   