#include "ScaleGizmoRenderer.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

namespace
{
    void buildCubeVertices(float halfSize, float* outVertices)
    {
        const float h = halfSize;

        const float vertices[] =
        {
            -h, -h, -h,
             h, -h, -h,
             h,  h, -h,
            -h,  h, -h,

            -h, -h,  h,
             h, -h,  h,
             h,  h,  h,
            -h,  h,  h
        };

        for (int i = 0; i < 24; i++)
        {
            outVertices[i] = vertices[i];
        }
    }
}

ScaleGizmoRenderer::ScaleGizmoRenderer()
    : m_LineShader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\transformGizmo\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\transformGizmo\\fragment.glsl"
    ),
    m_HandleShader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\transformGizmo\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\transformGizmo\\fragment.glsl"
    ),
    m_LineVBO(nullptr),
    m_HandleVBO(nullptr),
    m_HandleEBO(nullptr),
    m_GizmoSize(1.5f),
    m_HandleSize(0.12f)
{
    float lineVertices[] =
    {
        0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        m_GizmoSize, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, m_GizmoSize, 0.0f, 0.0f, 1.0f, 0.0f,

        0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, m_GizmoSize, 0.0f, 0.0f, 1.0f
    };

    m_LineVAO.bind();
    m_LineVBO = new VertexBuffer(lineVertices, sizeof(lineVertices));
    m_LineVBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_LineVAO.unbind();
    m_LineVBO->unbind();

    float cubeVertices[24];
    buildCubeVertices(m_HandleSize, cubeVertices);

    unsigned int cubeIndices[] =
    {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 4, 7, 7, 3, 0,
        1, 5, 6, 6, 2, 1,
        3, 2, 6, 6, 7, 3,
        0, 1, 5, 5, 4, 0
    };

    m_HandleVAO.bind();

    m_HandleVBO = new VertexBuffer(cubeVertices, sizeof(cubeVertices));
    m_HandleEBO = new IndexBuffer(cubeIndices, sizeof(cubeIndices) / sizeof(unsigned int));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    m_HandleVAO.unbind();
    m_HandleVBO->unbind();
    m_HandleEBO->unbind();
}

ScaleGizmoRenderer::~ScaleGizmoRenderer()
{
    delete m_LineVBO;
    delete m_HandleVBO;
    delete m_HandleEBO;
}

void ScaleGizmoRenderer::render(
    const SceneObject& selectedObject,
    const Camera& camera,
    TransformAxis activeAxis,
    TransformSpace transformSpace)
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

    float lineVertices[] =
    {
        0.0f, 0.0f, 0.0f, finalXColor.r, finalXColor.g, finalXColor.b,
        m_GizmoSize, 0.0f, 0.0f, finalXColor.r, finalXColor.g, finalXColor.b,

        0.0f, 0.0f, 0.0f, finalYColor.r, finalYColor.g, finalYColor.b,
        0.0f, m_GizmoSize, 0.0f, finalYColor.r, finalYColor.g, finalYColor.b,

        0.0f, 0.0f, 0.0f, finalZColor.r, finalZColor.g, finalZColor.b,
        0.0f, 0.0f, m_GizmoSize, finalZColor.r, finalZColor.g, finalZColor.b
    };

    glm::mat4 gizmoModel(1.0f);
    const glm::vec3& position = selectedObject.getTransform().getPosition();
    const glm::vec3& rotation = selectedObject.getTransform().getRotation();

    gizmoModel = glm::translate(gizmoModel, position);

    if (transformSpace == TransformSpace::Local)
    {
        gizmoModel = glm::rotate(gizmoModel, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        gizmoModel = glm::rotate(gizmoModel, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        gizmoModel = glm::rotate(gizmoModel, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    m_LineVBO->bind();
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(lineVertices), lineVertices);

    m_LineShader.use();
    m_LineShader.setMat4("u_View", camera.getViewMatrix());
    m_LineShader.setMat4("u_Projection", camera.getProjectionMatrix());
    m_LineShader.setMat4("u_Model", gizmoModel);

    m_LineVAO.bind();
    glDrawArrays(GL_LINES, 0, 6);

    glm::vec3 localHandlePositions[3] =
    {
        glm::vec3(m_GizmoSize, 0.0f, 0.0f),
        glm::vec3(0.0f, m_GizmoSize, 0.0f),
        glm::vec3(0.0f, 0.0f, m_GizmoSize)
    };

    glm::vec3 handleColors[3] =
    {
        finalXColor,
        finalYColor,
        finalZColor
    };

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    for (int i = 0; i < 3; i++)
    {
        glm::mat4 handleModel = gizmoModel;
        handleModel = glm::translate(handleModel, localHandlePositions[i]);

        m_HandleShader.use();
        m_HandleShader.setMat4("u_View", camera.getViewMatrix());
        m_HandleShader.setMat4("u_Projection", camera.getProjectionMatrix());
        m_HandleShader.setMat4("u_Model", handleModel);
        m_HandleShader.setVec3("u_Color", handleColors[i]);

        m_HandleVAO.bind();
        m_HandleEBO->bind();
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    }
}