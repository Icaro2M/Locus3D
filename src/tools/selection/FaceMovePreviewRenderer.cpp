#include "FaceMovePreviewRenderer.h"

#include <vector>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "../../resources/AssetPaths.h"

#include "../../scene/SceneObject.h"
#include "../../math/Transform.h"
#include "FaceGeometry.h"

FaceMovePreviewRenderer::FaceMovePreviewRenderer()
    : m_VAO(0),
    m_VBO(0),
    m_Shader(
        AssetPaths::shader("basic/vertex.glsl"),
        AssetPaths::shader("basic/fragment.glsl")
    )
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

FaceMovePreviewRenderer::~FaceMovePreviewRenderer()
{
    if (m_VBO != 0)
    {
        glDeleteBuffers(1, &m_VBO);
    }

    if (m_VAO != 0)
    {
        glDeleteVertexArrays(1, &m_VAO);
    }
}

void FaceMovePreviewRenderer::buildFillVertices(
    const FaceMoveTool& tool,
    std::vector<glm::vec3>& outVertices
) const
{
    outVertices.clear();

    if (!tool.isActive())
    {
        return;
    }

    const FaceGeometry& geometry = tool.getBaseGeometry();

    if (!geometry.isValid())
    {
        return;
    }

    SceneObject* object = geometry.getObject();
    if (object == nullptr)
    {
        return;
    }

    const glm::vec3 localV0 = geometry.getLocalV0();
    const glm::vec3 localV1 = geometry.getLocalV1();
    const glm::vec3 localV2 = geometry.getLocalV2();
    const glm::vec3 localNormal = geometry.getLocalNormal();

    const glm::mat4 modelMatrix = object->getTransform().getModelMatrix();

    const glm::vec3 worldV0 = glm::vec3(modelMatrix * glm::vec4(localV0, 1.0f));
    const glm::vec3 worldV1 = glm::vec3(modelMatrix * glm::vec4(localV1, 1.0f));
    const glm::vec3 worldV2 = glm::vec3(modelMatrix * glm::vec4(localV2, 1.0f));

    const glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
    glm::vec3 worldNormal = normalMatrix * localNormal;

    if (glm::length(worldNormal) <= 0.0001f)
    {
        return;
    }

    worldNormal = glm::normalize(worldNormal);

    const float distance = tool.getCurrentDistance();
    const glm::vec3 offset = worldNormal * distance;

    outVertices.push_back(worldV0 + offset);
    outVertices.push_back(worldV1 + offset);
    outVertices.push_back(worldV2 + offset);
}

void FaceMovePreviewRenderer::buildLineVertices(
    const FaceMoveTool& tool,
    std::vector<glm::vec3>& outVertices
) const
{
    outVertices.clear();

    if (!tool.isActive())
    {
        return;
    }

    const FaceGeometry& geometry = tool.getBaseGeometry();

    if (!geometry.isValid())
    {
        return;
    }

    SceneObject* object = geometry.getObject();
    if (object == nullptr)
    {
        return;
    }

    const glm::vec3 localV0 = geometry.getLocalV0();
    const glm::vec3 localV1 = geometry.getLocalV1();
    const glm::vec3 localV2 = geometry.getLocalV2();
    const glm::vec3 localNormal = geometry.getLocalNormal();

    const glm::mat4 modelMatrix = object->getTransform().getModelMatrix();

    const glm::vec3 worldV0 = glm::vec3(modelMatrix * glm::vec4(localV0, 1.0f));
    const glm::vec3 worldV1 = glm::vec3(modelMatrix * glm::vec4(localV1, 1.0f));
    const glm::vec3 worldV2 = glm::vec3(modelMatrix * glm::vec4(localV2, 1.0f));

    const glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
    glm::vec3 worldNormal = normalMatrix * localNormal;

    if (glm::length(worldNormal) <= 0.0001f)
    {
        return;
    }

    worldNormal = glm::normalize(worldNormal);

    const float distance = tool.getCurrentDistance();
    const glm::vec3 offset = worldNormal * distance;

    const glm::vec3 movedV0 = worldV0 + offset;
    const glm::vec3 movedV1 = worldV1 + offset;
    const glm::vec3 movedV2 = worldV2 + offset;

    outVertices.push_back(movedV0);
    outVertices.push_back(movedV1);

    outVertices.push_back(movedV1);
    outVertices.push_back(movedV2);

    outVertices.push_back(movedV2);
    outVertices.push_back(movedV0);
}

void FaceMovePreviewRenderer::render(const FaceMoveTool& tool, const Camera& camera)
{
    if (!tool.isActive())
    {
        return;
    }

    std::vector<glm::vec3> fillVertices;
    buildFillVertices(tool, fillVertices);

    if (fillVertices.empty())
    {
        return;
    }

    m_Shader.use();
    m_Shader.setMat4("u_Model", glm::mat4(1.0f));
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(fillVertices.size() * sizeof(glm::vec3)),
        fillVertices.data(),
        GL_DYNAMIC_DRAW
    );

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(fillVertices.size()));

    glDisable(GL_POLYGON_OFFSET_FILL);

    std::vector<glm::vec3> lineVertices;
    buildLineVertices(tool, lineVertices);

    if (!lineVertices.empty())
    {
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(lineVertices.size() * sizeof(glm::vec3)),
            lineVertices.data(),
            GL_DYNAMIC_DRAW
        );

        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size()));
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}