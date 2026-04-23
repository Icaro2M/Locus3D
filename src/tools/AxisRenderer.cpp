#include "AxisRenderer.h"

#include "../resources/AssetPaths.h"

#include <glad/glad.h>

#include <glm/glm/glm.hpp>

AxisRenderer::AxisRenderer()
    : m_VBO(nullptr),
    m_Shader(
        AssetPaths::shader("helpers/axis/axisVertex.glsl"),
        AssetPaths::shader("helpers/axis/axisFragment.glsl")
    )
{
    const float axisLength = 80.0f;

    const float xStrength = 1.0f;
    const float yStrength = 1.0f;
    const float zStrength = 1.0f;

    float vertices[] =
    {
        -axisLength, 0.0f, 0.0f,   0.85f, 0.24f, 0.24f,   xStrength,
         axisLength, 0.0f, 0.0f,   0.85f, 0.24f, 0.24f,   xStrength,

         0.0f, -axisLength, 0.0f,  0.20f, 0.82f, 0.35f,   yStrength,
         0.0f,  axisLength, 0.0f,  0.20f, 0.82f, 0.35f,   yStrength,

         0.0f, 0.0f, -axisLength,  0.22f, 0.48f, 0.90f,   zStrength,
         0.0f, 0.0f,  axisLength,  0.22f, 0.48f, 0.90f,   zStrength
    };

    m_VAO.bind();

    m_VBO = new VertexBuffer(vertices, sizeof(vertices));
    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

AxisRenderer::~AxisRenderer()
{
    delete m_VBO;
}

void AxisRenderer::render(const Camera& camera)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());
    m_Shader.setMat4("u_Model", glm::mat4(1.0f));

    glm::vec3 camPos = camera.getPosition();
    m_Shader.setFloat("u_CamX", camPos.x);
    m_Shader.setFloat("u_CamY", camPos.y);
    m_Shader.setFloat("u_CamZ", camPos.z);

    m_Shader.setFloat("u_Alpha", 0.95f);
    m_Shader.setFloat("u_FadeStart", 40.0f);
    m_Shader.setFloat("u_FadeEnd", 75.0f);
    m_Shader.setFloat("u_MinAlpha", 0.0f);

    m_VAO.bind();

    glLineWidth(2.0f);
    glDrawArrays(GL_LINES, 0, m_VertexCount);
    glLineWidth(1.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}