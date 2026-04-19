#include "PushPullPreviewRenderer.h"

#include <vector>

#include <glad/glad.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_inverse.hpp>

#include "../../resources/AssetPaths.h"
#include "../../scene/SceneObject.h"
#include "../../math/Transform.h"
#include "FaceGeometry.h"

PushPullPreviewRenderer::PushPullPreviewRenderer()
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

PushPullPreviewRenderer::~PushPullPreviewRenderer()
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

void PushPullPreviewRenderer::buildFillVertices(
    const PushPullTool& tool,
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

    const std::vector<glm::vec3>& localBoundaryVertices =
        geometry.getLocalBoundaryVertices();

    if (localBoundaryVertices.size() < 3)
    {
        return;
    }

    const glm::vec3 localNormal = geometry.getLocalNormal();
    const glm::mat4 modelMatrix = object->getTransform().getModelMatrix();
    const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));

    glm::vec3 worldNormal = normalMatrix * localNormal;
    if (glm::length(worldNormal) <= 0.0001f)
    {
        return;
    }

    worldNormal = glm::normalize(worldNormal);

    const float distance = tool.getCurrentDistance();
    const glm::vec3 offset = worldNormal * distance;

    std::vector<glm::vec3> worldBaseVertices;
    std::vector<glm::vec3> worldTopVertices;

    worldBaseVertices.reserve(localBoundaryVertices.size());
    worldTopVertices.reserve(localBoundaryVertices.size());

    for (const glm::vec3& localVertex : localBoundaryVertices)
    {
        glm::vec3 worldVertex =
            glm::vec3(modelMatrix * glm::vec4(localVertex, 1.0f));

        worldBaseVertices.push_back(worldVertex);
        worldTopVertices.push_back(worldVertex + offset);
    }

    // tampa superior triangulada em leque
    for (unsigned int i = 1; i + 1 < worldTopVertices.size(); ++i)
    {
        outVertices.push_back(worldTopVertices[0]);
        outVertices.push_back(worldTopVertices[i]);
        outVertices.push_back(worldTopVertices[i + 1]);
    }

    // laterais
    const unsigned int boundaryCount = static_cast<unsigned int>(worldBaseVertices.size());

    for (unsigned int i = 0; i < boundaryCount; ++i)
    {
        unsigned int next = (i + 1) % boundaryCount;

        const glm::vec3& baseV0 = worldBaseVertices[i];
        const glm::vec3& baseV1 = worldBaseVertices[next];
        const glm::vec3& topV0 = worldTopVertices[i];
        const glm::vec3& topV1 = worldTopVertices[next];

        outVertices.push_back(baseV0);
        outVertices.push_back(baseV1);
        outVertices.push_back(topV1);

        outVertices.push_back(baseV0);
        outVertices.push_back(topV1);
        outVertices.push_back(topV0);
    }
}

void PushPullPreviewRenderer::buildLineVertices(
    const PushPullTool& tool,
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

    const std::vector<glm::vec3>& localBoundaryVertices =
        geometry.getLocalBoundaryVertices();

    if (localBoundaryVertices.size() < 3)
    {
        return;
    }

    const glm::vec3 localNormal = geometry.getLocalNormal();
    const glm::mat4 modelMatrix = object->getTransform().getModelMatrix();
    const glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(modelMatrix));

    glm::vec3 worldNormal = normalMatrix * localNormal;
    if (glm::length(worldNormal) <= 0.0001f)
    {
        return;
    }

    worldNormal = glm::normalize(worldNormal);

    const float distance = tool.getCurrentDistance();
    const glm::vec3 offset = worldNormal * distance;

    std::vector<glm::vec3> worldBaseVertices;
    std::vector<glm::vec3> worldTopVertices;

    worldBaseVertices.reserve(localBoundaryVertices.size());
    worldTopVertices.reserve(localBoundaryVertices.size());

    for (const glm::vec3& localVertex : localBoundaryVertices)
    {
        glm::vec3 worldVertex =
            glm::vec3(modelMatrix * glm::vec4(localVertex, 1.0f));

        worldBaseVertices.push_back(worldVertex);
        worldTopVertices.push_back(worldVertex + offset);
    }

    const unsigned int boundaryCount = static_cast<unsigned int>(worldTopVertices.size());

    // contorno do topo
    for (unsigned int i = 0; i < boundaryCount; ++i)
    {
        unsigned int next = (i + 1) % boundaryCount;

        outVertices.push_back(worldTopVertices[i]);
        outVertices.push_back(worldTopVertices[next]);
    }

    // arestas verticais
    for (unsigned int i = 0; i < boundaryCount; ++i)
    {
        outVertices.push_back(worldBaseVertices[i]);
        outVertices.push_back(worldTopVertices[i]);
    }
}

void PushPullPreviewRenderer::render(const PushPullTool& tool, const Camera& camera)
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