#include "FaceScalePreviewRenderer.h"

#include <glad/glad.h>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "../../resources/AssetPaths.h"
#include "../../scene/SceneObject.h"
#include "../../math/Transform.h"
#include "FaceGeometry.h"

FaceScalePreviewRenderer::FaceScalePreviewRenderer()
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

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec3),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

FaceScalePreviewRenderer::~FaceScalePreviewRenderer()
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

void FaceScalePreviewRenderer::buildFillVertices(
    const FaceScaleTool& tool,
    const Camera& camera,
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

    const glm::vec3 localCenter = geometry.getLocalCenter();
    const float scaleFactor = tool.getCurrentScaleFactor();

    glm::vec3 localNormal = geometry.getLocalNormal();
    if (glm::length(localNormal) > 0.00001f)
    {
        localNormal = glm::normalize(localNormal);
    }

    const glm::mat4 modelMatrix =
        object->getTransform().getModelMatrix();

    const glm::vec3 worldCenter =
        glm::vec3(modelMatrix * glm::vec4(localCenter, 1.0f));

    glm::vec3 worldNormal = glm::mat3(modelMatrix) * localNormal;
    if (glm::length(worldNormal) > 0.00001f)
    {
        worldNormal = glm::normalize(worldNormal);
    }

    glm::vec3 viewDirection = camera.getPosition() - worldCenter;
    if (glm::length(viewDirection) > 0.00001f)
    {
        viewDirection = glm::normalize(viewDirection);
    }

    if (glm::dot(worldNormal, viewDirection) < 0.0f)
    {
        worldNormal = -worldNormal;
    }

    const float previewOffset = 0.01f;

    std::vector<glm::vec3> scaledWorldVertices;
    scaledWorldVertices.reserve(localBoundaryVertices.size());

    for (const glm::vec3& localVertex : localBoundaryVertices)
    {
        const glm::vec3 localOffset = localVertex - localCenter;

        const glm::vec3 scaledLocalVertex =
            localCenter + localOffset * scaleFactor;

        glm::vec3 worldVertex =
            glm::vec3(modelMatrix * glm::vec4(scaledLocalVertex, 1.0f));

        worldVertex += worldNormal * previewOffset;

        scaledWorldVertices.push_back(worldVertex);
    }

    for (unsigned int i = 1; i + 1 < scaledWorldVertices.size(); ++i)
    {
        outVertices.push_back(scaledWorldVertices[0]);
        outVertices.push_back(scaledWorldVertices[i]);
        outVertices.push_back(scaledWorldVertices[i + 1]);
    }
}

void FaceScalePreviewRenderer::buildLineVertices(
    const FaceScaleTool& tool,
    const Camera& camera,
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

    const glm::vec3 localCenter = geometry.getLocalCenter();
    const float scaleFactor = tool.getCurrentScaleFactor();

    glm::vec3 localNormal = geometry.getLocalNormal();
    if (glm::length(localNormal) > 0.00001f)
    {
        localNormal = glm::normalize(localNormal);
    }

    const glm::mat4 modelMatrix =
        object->getTransform().getModelMatrix();

    const glm::vec3 worldCenter =
        glm::vec3(modelMatrix * glm::vec4(localCenter, 1.0f));

    glm::vec3 worldNormal = glm::mat3(modelMatrix) * localNormal;
    if (glm::length(worldNormal) > 0.00001f)
    {
        worldNormal = glm::normalize(worldNormal);
    }

    glm::vec3 viewDirection = camera.getPosition() - worldCenter;
    if (glm::length(viewDirection) > 0.00001f)
    {
        viewDirection = glm::normalize(viewDirection);
    }

    if (glm::dot(worldNormal, viewDirection) < 0.0f)
    {
        worldNormal = -worldNormal;
    }

    const float previewOffset = 0.01f;

    std::vector<glm::vec3> scaledWorldVertices;
    scaledWorldVertices.reserve(localBoundaryVertices.size());

    for (const glm::vec3& localVertex : localBoundaryVertices)
    {
        const glm::vec3 localOffset = localVertex - localCenter;

        const glm::vec3 scaledLocalVertex =
            localCenter + localOffset * scaleFactor;

        glm::vec3 worldVertex =
            glm::vec3(modelMatrix * glm::vec4(scaledLocalVertex, 1.0f));

        worldVertex += worldNormal * previewOffset;

        scaledWorldVertices.push_back(worldVertex);
    }

    const unsigned int boundaryCount =
        static_cast<unsigned int>(scaledWorldVertices.size());

    for (unsigned int i = 0; i < boundaryCount; ++i)
    {
        const unsigned int next = (i + 1) % boundaryCount;

        outVertices.push_back(scaledWorldVertices[i]);
        outVertices.push_back(scaledWorldVertices[next]);
    }
}

void FaceScalePreviewRenderer::render(
    const FaceScaleTool& tool,
    const Camera& camera
)
{
    if (!tool.isActive())
    {
        return;
    }

    std::vector<glm::vec3> fillVertices;
    buildFillVertices(tool, camera, fillVertices);

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
    buildLineVertices(tool, camera, lineVertices);

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