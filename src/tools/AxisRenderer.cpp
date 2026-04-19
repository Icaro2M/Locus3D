#include "AxisRenderer.h"

#include "../resources/AssetPaths.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>

AxisRenderer::AxisRenderer()
    : m_VBO(nullptr),
    m_Shader(
        AssetPaths::shader("helpers/axis/vertex.glsl"),
        AssetPaths::shader("helpers/axis/fragment.glsl")
    )
{
    float axisLength = 500.0f;

    float vertices[] =
    {
        // X - vermelho
        -axisLength, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,
         axisLength, 0.0f, 0.0f,   1.0f, 0.0f, 0.0f,

         // Y - verde
         0.0f, -axisLength, 0.0f,   0.0f, 1.0f, 0.0f,
         0.0f,  axisLength, 0.0f,   0.0f, 1.0f, 0.0f,

         // Z - azul
         0.0f, 0.0f, -axisLength,   0.0f, 0.0f, 1.0f,
         0.0f, 0.0f,  axisLength,   0.0f, 0.0f, 1.0f
    };

    m_VAO.bind();

    m_VBO = new VertexBuffer(vertices, sizeof(vertices));
    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

AxisRenderer::~AxisRenderer()
{
    delete m_VBO;
}

void AxisRenderer::render(const Camera& camera)
{
    m_Shader.use();

    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());
    m_Shader.setMat4("u_Model", glm::mat4(1.0f));

    m_VAO.bind();

    glDrawArrays(GL_LINES, 0, m_VertexCount);
}