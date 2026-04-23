#include "SelectionOutlineRenderer.h"

#include "../../resources/AssetPaths.h"
#include "../../geometry/Mesh.h"
#include "../../geometry/LogicalFace.h"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include <vector>

namespace
{
    glm::vec3 computeObjectLocalCenter(const Mesh& mesh)
    {
        const std::vector<float>& vertices = mesh.getVertices();

        if (vertices.empty())
        {
            return glm::vec3(0.0f);
        }

        glm::vec3 center(0.0f);
        const unsigned int vertexCount = static_cast<unsigned int>(vertices.size() / 6);

        for (unsigned int i = 0; i < vertexCount; ++i)
        {
            center += mesh.getVertexPosition(i);
        }

        return center / static_cast<float>(vertexCount);
    }

    glm::vec3 computeLocalFaceCenter(const Mesh& mesh, const LogicalFace& logicalFace)
    {
        const std::vector<unsigned int>& boundary = logicalFace.getBoundaryVertexIndices();

        if (boundary.empty())
        {
            return glm::vec3(0.0f);
        }

        glm::vec3 center(0.0f);

        for (unsigned int vertexIndex : boundary)
        {
            center += mesh.getVertexPosition(vertexIndex);
        }

        return center / static_cast<float>(boundary.size());
    }

    glm::vec3 computeRawLocalFaceNormal(const Mesh& mesh, const LogicalFace& logicalFace)
    {
        const std::vector<unsigned int>& triangleIndices = logicalFace.getTriangleIndices();

        for (unsigned int triangleIndex : triangleIndices)
        {
            unsigned int i0 = 0;
            unsigned int i1 = 0;
            unsigned int i2 = 0;

            if (!mesh.getTriangleVertexIndices(triangleIndex, i0, i1, i2))
            {
                continue;
            }

            const glm::vec3 v0 = mesh.getVertexPosition(i0);
            const glm::vec3 v1 = mesh.getVertexPosition(i1);
            const glm::vec3 v2 = mesh.getVertexPosition(i2);

            const glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
            const float lenSq = glm::dot(normal, normal);

            if (lenSq > 0.000001f)
            {
                return glm::normalize(normal);
            }
        }

        const std::vector<unsigned int>& boundary = logicalFace.getBoundaryVertexIndices();

        if (boundary.size() >= 3)
        {
            const glm::vec3 v0 = mesh.getVertexPosition(boundary[0]);
            const glm::vec3 v1 = mesh.getVertexPosition(boundary[1]);
            const glm::vec3 v2 = mesh.getVertexPosition(boundary[2]);

            const glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);
            const float lenSq = glm::dot(normal, normal);

            if (lenSq > 0.000001f)
            {
                return glm::normalize(normal);
            }
        }

        return glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 computeOutwardLocalFaceNormal(
        const Mesh& mesh,
        const LogicalFace& logicalFace,
        const glm::vec3& objectLocalCenter
    )
    {
        glm::vec3 normal = computeRawLocalFaceNormal(mesh, logicalFace);
        const glm::vec3 faceCenter = computeLocalFaceCenter(mesh, logicalFace);
        const glm::vec3 outwardHint = faceCenter - objectLocalCenter;

        const float hintLenSq = glm::dot(outwardHint, outwardHint);

        if (hintLenSq > 0.000001f && glm::dot(normal, outwardHint) < 0.0f)
        {
            normal = -normal;
        }

        return normal;
    }

    bool isFaceFrontFacing(
        const Mesh& mesh,
        const LogicalFace& logicalFace,
        const glm::mat4& modelMatrix,
        const glm::mat3& normalMatrix,
        const glm::vec3& objectLocalCenter,
        const Camera& camera
    )
    {
        const glm::vec3 localCenter = computeLocalFaceCenter(mesh, logicalFace);
        const glm::vec3 localNormal = computeOutwardLocalFaceNormal(mesh, logicalFace, objectLocalCenter);

        glm::vec3 worldNormal = normalMatrix * localNormal;
        const float normalLenSq = glm::dot(worldNormal, worldNormal);

        if (normalLenSq <= 0.000001f)
        {
            return true;
        }

        worldNormal = glm::normalize(worldNormal);

        const glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(localCenter, 1.0f));
        const glm::vec3 toCamera = camera.getPosition() - worldCenter;
        const float toCameraLenSq = glm::dot(toCamera, toCamera);

        if (toCameraLenSq <= 0.000001f)
        {
            return true;
        }

        const glm::vec3 viewDir = glm::normalize(toCamera);

        return glm::dot(worldNormal, viewDir) > 0.0f;
    }

    float computeCameraOffsetDistance(const glm::vec3& worldPoint, const Camera& camera)
    {
        const float distanceToCamera = glm::length(camera.getPosition() - worldPoint);

        float offset = distanceToCamera * 0.0005f;

        if (offset < 0.0005f)
        {
            offset = 0.0005f;
        }

        if (offset > 0.0050f)
        {
            offset = 0.0050f;
        }

        return offset;
    }
}

SelectionOutlineRenderer::SelectionOutlineRenderer()
    : m_Shader(
        AssetPaths::shader("helpers/selection/selectionOutlineVertex.glsl"),
        AssetPaths::shader("helpers/selection/selectionOutlineFragment.glsl")
    ),
    m_VBO(nullptr)
{
    float initialVertices[] = { 0.0f, 0.0f, 0.0f };

    m_VAO.bind();

    m_VBO = new VertexBuffer(initialVertices, sizeof(initialVertices));
    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    m_VAO.unbind();
    m_VBO->unbind();
}

SelectionOutlineRenderer::~SelectionOutlineRenderer()
{
    delete m_VBO;
}

void SelectionOutlineRenderer::render(const SceneObject& selectedObject, const Camera& camera)
{
    const Mesh& mesh = selectedObject.getMesh();

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

    const glm::mat4 modelMatrix = selectedObject.getTransform().getModelMatrix();

    if (!mesh.hasLogicalFaces())
    {
        glm::mat4 outlineModel = glm::scale(modelMatrix, glm::vec3(1.02f, 1.02f, 1.02f));

        m_Shader.setMat4("u_Model", outlineModel);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        mesh.bind();
        glDrawElements(
            GL_TRIANGLES,
            mesh.getIndexBuffer().getCount(),
            GL_UNSIGNED_INT,
            nullptr
        );

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        return;
    }

    const glm::mat3 normalMatrix = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
    const glm::vec3 objectLocalCenter = computeObjectLocalCenter(mesh);
    const std::vector<LogicalFace>& logicalFaces = mesh.getLogicalFaces();

    std::vector<float> lineVertices;
    lineVertices.reserve(logicalFaces.size() * 24);

    for (const LogicalFace& logicalFace : logicalFaces)
    {
        const std::vector<unsigned int>& boundary = logicalFace.getBoundaryVertexIndices();

        if (boundary.size() < 2)
        {
            continue;
        }

        if (!isFaceFrontFacing(mesh, logicalFace, modelMatrix, normalMatrix, objectLocalCenter, camera))
        {
            continue;
        }

        for (unsigned int i = 0; i < boundary.size(); ++i)
        {
            const unsigned int next = (i + 1) % static_cast<unsigned int>(boundary.size());

            const glm::vec3 localV0 = mesh.getVertexPosition(boundary[i]);
            const glm::vec3 localV1 = mesh.getVertexPosition(boundary[next]);

            glm::vec3 worldV0 = glm::vec3(modelMatrix * glm::vec4(localV0, 1.0f));
            glm::vec3 worldV1 = glm::vec3(modelMatrix * glm::vec4(localV1, 1.0f));

            glm::vec3 toCamera0 = camera.getPosition() - worldV0;
            glm::vec3 toCamera1 = camera.getPosition() - worldV1;

            const float lenSq0 = glm::dot(toCamera0, toCamera0);
            const float lenSq1 = glm::dot(toCamera1, toCamera1);

            if (lenSq0 > 0.000001f)
            {
                worldV0 += glm::normalize(toCamera0) * computeCameraOffsetDistance(worldV0, camera);
            }

            if (lenSq1 > 0.000001f)
            {
                worldV1 += glm::normalize(toCamera1) * computeCameraOffsetDistance(worldV1, camera);
            }

            lineVertices.push_back(worldV0.x);
            lineVertices.push_back(worldV0.y);
            lineVertices.push_back(worldV0.z);

            lineVertices.push_back(worldV1.x);
            lineVertices.push_back(worldV1.y);
            lineVertices.push_back(worldV1.z);
        }
    }

    if (lineVertices.empty())
    {
        return;
    }

    delete m_VBO;

    m_VAO.bind();

    m_VBO = new VertexBuffer(
        lineVertices.data(),
        static_cast<unsigned int>(lineVertices.size() * sizeof(float))
    );
    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    m_VAO.unbind();

    m_Shader.setMat4("u_Model", glm::mat4(1.0f));

    glLineWidth(1.0f);

    m_VAO.bind();
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size() / 3));
}