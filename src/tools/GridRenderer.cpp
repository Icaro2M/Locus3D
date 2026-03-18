#include "GridRenderer.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <vector>

GridRenderer::GridRenderer()
    : m_VBO(nullptr),
    m_Shader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\grid\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\grid\\fragment.glsl"
    ),
    m_VertexCount(0)
{
    const float gridExtent = 100.0f;
    const float lineSpacing = 5.0f;
    const int gridDivisions = 20;

    std::vector<float> vertices;

    for (int i = -gridDivisions; i <= gridDivisions; i++)
    {
        const float lineOffset = i * lineSpacing;


        vertices.push_back(-gridExtent);
        vertices.push_back(0.0f);
        vertices.push_back(lineOffset);
        vertices.push_back(0.6f);
        vertices.push_back(0.6f);
        vertices.push_back(0.6f);

        vertices.push_back(gridExtent);
        vertices.push_back(0.0f);
        vertices.push_back(lineOffset);
        vertices.push_back(0.6f);
        vertices.push_back(0.6f);
        vertices.push_back(0.6f);


        vertices.push_back(lineOffset);
        vertices.push_back(0.0f);
        vertices.push_back(-gridExtent);
        vertices.push_back(0.6f);
        vertices.push_back(0.6f);
        vertices.push_back(0.6f);

        vertices.push_back(lineOffset);
        vertices.push_back(0.0f);
        vertices.push_back(gridExtent);
        vertices.push_back(0.6f);
        vertices.push_back(0.6f);
        vertices.push_back(0.6f);
    }

    m_VertexCount = static_cast<unsigned int>(vertices.size() / 6);

    m_VAO.bind();

    m_VBO = new VertexBuffer(vertices.data(), static_cast<unsigned int>(vertices.size() * sizeof(float)));
    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

GridRenderer::~GridRenderer()
{
    delete m_VBO;
}

void GridRenderer::render(const Camera& camera)
{
    m_Shader.use();

    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());
    m_Shader.setMat4("u_Model", glm::mat4(1.0f));

    m_VAO.bind();

    glDrawArrays(GL_LINES, 0, m_VertexCount);
}