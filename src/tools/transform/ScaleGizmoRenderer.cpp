#include "ScaleGizmoRenderer.h"

#include "../../resources/AssetPaths.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "../../geometry/primitives/PrimitiveFactory.h"

std::vector<float> ScaleGizmoRenderer::buildColoredVertices(const glm::vec3& color) const
{
    std::vector<float> result;
    result.reserve((m_HandleBaseVertices.size() / 6) * 6);

    for (size_t i = 0; i < m_HandleBaseVertices.size(); i += 6)
    {
        result.push_back(m_HandleBaseVertices[i + 0]);
        result.push_back(m_HandleBaseVertices[i + 1]);
        result.push_back(m_HandleBaseVertices[i + 2]);

        result.push_back(color.r);
        result.push_back(color.g);
        result.push_back(color.b);
    }

    return result;
}

ScaleGizmoRenderer::ScaleGizmoRenderer()
    : m_Shader(
        AssetPaths::shader("helpers/transformGizmo/vertex.glsl"),
        AssetPaths::shader("helpers/transformGizmo/fragment.glsl")
    ),
    m_LineVBO(nullptr),
    m_HandleVBO(nullptr),
    m_HandleEBO(nullptr),
    m_GizmoSize(1.5f),
    m_HandleSize(0.12f),
    m_HandleIndexCount(0)
{

    float lineVertices[] =
    {
        0,0,0, 1,0,0,   m_GizmoSize,0,0, 1,0,0,
        0,0,0, 0,1,0,   0,m_GizmoSize,0, 0,1,0,
        0,0,0, 0,0,1,   0,0,m_GizmoSize, 0,0,1
    };

    m_LineVAO.bind();
    m_LineVBO = new VertexBuffer(lineVertices, sizeof(lineVertices));

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_LineVAO.unbind();

    auto cube = PrimitiveFactory::createCube();

    m_HandleBaseVertices = cube.getVertices();
    const auto& indices = cube.getIndices();

    std::vector<float> initial =
        buildColoredVertices(glm::vec3(1, 0, 0));

    m_HandleIndexCount = (unsigned int)indices.size();

    m_HandleVAO.bind();

    m_HandleVBO = new VertexBuffer(
        initial.data(),
        (unsigned int)(initial.size() * sizeof(float))
    );

    m_HandleEBO = new IndexBuffer(
        indices.data(),
        (unsigned int)indices.size()
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_HandleVAO.unbind();
}

ScaleGizmoRenderer::~ScaleGizmoRenderer()
{
    delete m_LineVBO;
    delete m_HandleVBO;
    delete m_HandleEBO;
}

void ScaleGizmoRenderer::render(
    const SceneObject& obj,
    const Camera& camera,
    TransformAxis activeAxis,
    TransformSpace space)
{
    glm::vec3 x(1, 0, 0), y(0, 1, 0), z(0, 0, 1);
    glm::vec3 dx(0.35f, 0, 0), dy(0, 0.35f, 0), dz(0, 0, 0.35f);

    glm::vec3 cx = x, cy = y, cz = z;

    if (activeAxis == TransformAxis::X) { cy = dy; cz = dz; }
    if (activeAxis == TransformAxis::Y) { cx = dx; cz = dz; }
    if (activeAxis == TransformAxis::Z) { cx = dx; cy = dy; }

    float line[] =
    {
        0,0,0, cx.r,cx.g,cx.b, m_GizmoSize,0,0, cx.r,cx.g,cx.b,
        0,0,0, cy.r,cy.g,cy.b, 0,m_GizmoSize,0, cy.r,cy.g,cy.b,
        0,0,0, cz.r,cz.g,cz.b, 0,0,m_GizmoSize, cz.r,cz.g,cz.b
    };

    glm::mat4 model(1.0f);
    model = glm::translate(model, obj.getTransform().getPosition());

    if (space == TransformSpace::Local)
    {
        auto r = obj.getTransform().getRotation();
        model = glm::rotate(model, glm::radians(r.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(r.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(r.z), glm::vec3(0, 0, 1));
    }

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

    m_LineVBO->bind();
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(line), line);

    m_Shader.setMat4("u_Model", model);
    m_LineVAO.bind();
    glDrawArrays(GL_LINES, 0, 6);

    glm::vec3 pos[3] =
    {
        {m_GizmoSize,0,0},
        {0,m_GizmoSize,0},
        {0,0,m_GizmoSize}
    };

    glm::vec3 col[3] = { cx,cy,cz };

    for (int i = 0; i < 3; i++)
    {
        auto verts = buildColoredVertices(col[i]);

        m_HandleVBO->bind();
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            (unsigned int)(verts.size() * sizeof(float)), verts.data());

        glm::mat4 m = model;
        m = glm::translate(m, pos[i]);
        m = glm::scale(m, glm::vec3(m_HandleSize * 2));

        m_Shader.setMat4("u_Model", m);

        m_HandleVAO.bind();
        m_HandleEBO->bind();
        glDrawElements(GL_TRIANGLES, m_HandleIndexCount, GL_UNSIGNED_INT, nullptr);
    }
}