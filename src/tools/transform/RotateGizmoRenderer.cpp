#include "RotateGizmoRenderer.h"

#include "../../resources/AssetPaths.h"

#include <vector>

RotateGizmoRenderer::RotateGizmoRenderer()
    : m_Shader(
        AssetPaths::shader("helpers/transformGizmo/vertex.glsl"),
        AssetPaths::shader("helpers/transformGizmo/fragment.glsl")
    ),
    m_VBO(nullptr),
    m_Radius(1.5f),
    m_Segments(64)
{
    std::vector<float> vertices;
    vertices.reserve(m_Segments * 2 * 6 * 3);

    for (int axisIndex = 0; axisIndex < 3; axisIndex++)
    {
        for (int i = 0; i < m_Segments; i++)
        {
            float a0 = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(m_Segments);
            float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(m_Segments);

            glm::vec3 p0(0.0f, 0.0f, 0.0f);
            glm::vec3 p1(0.0f, 0.0f, 0.0f);
            glm::vec3 color(1.0f, 1.0f, 1.0f);

            if (axisIndex == 0)
            {
                p0 = glm::vec3(0.0f, std::cos(a0) * m_Radius, std::sin(a0) * m_Radius);
                p1 = glm::vec3(0.0f, std::cos(a1) * m_Radius, std::sin(a1) * m_Radius);
                color = glm::vec3(1.0f, 0.0f, 0.0f);
            }
            else if (axisIndex == 1)
            {
                p0 = glm::vec3(std::cos(a0) * m_Radius, 0.0f, std::sin(a0) * m_Radius);
                p1 = glm::vec3(std::cos(a1) * m_Radius, 0.0f, std::sin(a1) * m_Radius);
                color = glm::vec3(0.0f, 1.0f, 0.0f);
            }
            else
            {
                p0 = glm::vec3(std::cos(a0) * m_Radius, std::sin(a0) * m_Radius, 0.0f);
                p1 = glm::vec3(std::cos(a1) * m_Radius, std::sin(a1) * m_Radius, 0.0f);
                color = glm::vec3(0.0f, 0.0f, 1.0f);
            }

            vertices.push_back(p0.x);
            vertices.push_back(p0.y);
            vertices.push_back(p0.z);
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);

            vertices.push_back(p1.x);
            vertices.push_back(p1.y);
            vertices.push_back(p1.z);
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }
    }

    m_VAO.bind();
    m_VBO = new VertexBuffer(vertices.data(), static_cast<unsigned int>(vertices.size() * sizeof(float)));
    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_VAO.unbind();
    m_VBO->unbind();
}

RotateGizmoRenderer::~RotateGizmoRenderer()
{
    delete m_VBO;
}

void RotateGizmoRenderer::render(
    const SceneObject& selectedObject,
    const Camera& camera,
    TransformAxis activeAxis,
    TransformSpace transformSpace
)
{
    glm::vec3 xColor(1.0f, 0.0f, 0.0f);
    glm::vec3 yColor(0.0f, 1.0f, 0.0f);
    glm::vec3 zColor(0.0f, 0.0f, 1.0f);

    glm::vec3 dimXColor(0.35f, 0.0f, 0.0f);
    glm::vec3 dimYColor(0.0f, 0.35f, 0.0f);
    glm::vec3 dimZColor(0.0f, 0.0f, 0.35f);

    glm::vec3 finalXColor = xColor;
    glm::vec3 finalYColor = yColor;
    glm::vec3 finalZColor = zColor;

    switch (activeAxis)
    {
    case TransformAxis::X:
        finalYColor = dimYColor;
        finalZColor = dimZColor;
        break;
    case TransformAxis::Y:
        finalXColor = dimXColor;
        finalZColor = dimZColor;
        break;
    case TransformAxis::Z:
        finalXColor = dimXColor;
        finalYColor = dimYColor;
        break;
    case TransformAxis::None:
        break;
    }

    std::vector<float> vertices;
    vertices.reserve(m_Segments * 2 * 6 * 3);

    for (int axisIndex = 0; axisIndex < 3; axisIndex++)
    {
        glm::vec3 color = finalXColor;
        if (axisIndex == 1) color = finalYColor;
        if (axisIndex == 2) color = finalZColor;

        for (int i = 0; i < m_Segments; i++)
        {
            float a0 = glm::two_pi<float>() * static_cast<float>(i) / static_cast<float>(m_Segments);
            float a1 = glm::two_pi<float>() * static_cast<float>(i + 1) / static_cast<float>(m_Segments);

            glm::vec3 p0(0.0f, 0.0f, 0.0f);
            glm::vec3 p1(0.0f, 0.0f, 0.0f);

            if (axisIndex == 0)
            {
                p0 = glm::vec3(0.0f, std::cos(a0) * m_Radius, std::sin(a0) * m_Radius);
                p1 = glm::vec3(0.0f, std::cos(a1) * m_Radius, std::sin(a1) * m_Radius);
            }
            else if (axisIndex == 1)
            {
                p0 = glm::vec3(std::cos(a0) * m_Radius, 0.0f, std::sin(a0) * m_Radius);
                p1 = glm::vec3(std::cos(a1) * m_Radius, 0.0f, std::sin(a1) * m_Radius);
            }
            else
            {
                p0 = glm::vec3(std::cos(a0) * m_Radius, std::sin(a0) * m_Radius, 0.0f);
                p1 = glm::vec3(std::cos(a1) * m_Radius, std::sin(a1) * m_Radius, 0.0f);
            }

            vertices.push_back(p0.x);
            vertices.push_back(p0.y);
            vertices.push_back(p0.z);
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);

            vertices.push_back(p1.x);
            vertices.push_back(p1.y);
            vertices.push_back(p1.z);
            vertices.push_back(color.r);
            vertices.push_back(color.g);
            vertices.push_back(color.b);
        }
    }

    m_VBO->bind();
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<unsigned int>(vertices.size() * sizeof(float)), vertices.data());

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

    glm::mat4 model(1.0f);
    const glm::vec3& position = selectedObject.getTransform().getPosition();
    const glm::vec3& rotation = selectedObject.getTransform().getRotation();

    model = glm::translate(model, position);

    if (transformSpace == TransformSpace::Local)
    {
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    m_Shader.setMat4("u_Model", model);

    m_VAO.bind();
    glDrawArrays(GL_LINES, 0, m_Segments * 2 * 3);
}