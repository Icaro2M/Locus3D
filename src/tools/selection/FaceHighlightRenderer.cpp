#include "FaceHighlightRenderer.h"

#include "../../geometry/LogicalFace.h"
#include "../../geometry/Mesh.h"
#include "../../resources/AssetPaths.h"

#include <glad/glad.h>

#include <vector>

FaceHighlightRenderer::FaceHighlightRenderer()
    : m_Shader(
        AssetPaths::shader("helpers/selection/faceSelectionVertex.glsl"),
        AssetPaths::shader("helpers/selection/faceSelectionFragment.glsl")
    ),
    m_VBO(nullptr)
{
    float initialVertices[] =
    {
        0.0f, 0.0f, 0.0f
    };

    m_VAO.bind();

    m_VBO = new VertexBuffer(initialVertices, sizeof(initialVertices));
    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    m_VAO.unbind();
    m_VBO->unbind();
}

FaceHighlightRenderer::~FaceHighlightRenderer()
{
    delete m_VBO;
}

void FaceHighlightRenderer::render(
    const SceneObject& object,
    int faceIndex,
    const Camera& camera
)
{
    if (faceIndex < 0)
    {
        return;
    }

    const Mesh& mesh = object.getMesh();

    const LogicalFace* logicalFace = nullptr;

    if (mesh.hasLogicalFaces())
    {
        logicalFace = mesh.getLogicalFace(static_cast<unsigned int>(faceIndex));
    }

    std::vector<float> highlightVertices;

    glm::mat4 modelMatrix = object.getTransform().getModelMatrix();

    if (logicalFace != nullptr)
    {
        const std::vector<unsigned int>& triangleIndices = logicalFace->getTriangleIndices();

        highlightVertices.reserve(triangleIndices.size() * 9);

        for (unsigned int triangleIndex : triangleIndices)
        {
            unsigned int i0 = 0;
            unsigned int i1 = 0;
            unsigned int i2 = 0;

            if (!mesh.getTriangleVertexIndices(triangleIndex, i0, i1, i2))
            {
                continue;
            }

            glm::vec3 localV0 = mesh.getVertexPosition(i0);
            glm::vec3 localV1 = mesh.getVertexPosition(i1);
            glm::vec3 localV2 = mesh.getVertexPosition(i2);

            glm::vec3 worldV0 = glm::vec3(modelMatrix * glm::vec4(localV0, 1.0f));
            glm::vec3 worldV1 = glm::vec3(modelMatrix * glm::vec4(localV1, 1.0f));
            glm::vec3 worldV2 = glm::vec3(modelMatrix * glm::vec4(localV2, 1.0f));

            highlightVertices.push_back(worldV0.x);
            highlightVertices.push_back(worldV0.y);
            highlightVertices.push_back(worldV0.z);

            highlightVertices.push_back(worldV1.x);
            highlightVertices.push_back(worldV1.y);
            highlightVertices.push_back(worldV1.z);

            highlightVertices.push_back(worldV2.x);
            highlightVertices.push_back(worldV2.y);
            highlightVertices.push_back(worldV2.z);
        }
    }
    else
    {
        unsigned int i0 = 0;
        unsigned int i1 = 0;
        unsigned int i2 = 0;

        if (!mesh.getTriangleVertexIndices(static_cast<unsigned int>(faceIndex), i0, i1, i2))
        {
            return;
        }

        glm::vec3 localV0 = mesh.getVertexPosition(i0);
        glm::vec3 localV1 = mesh.getVertexPosition(i1);
        glm::vec3 localV2 = mesh.getVertexPosition(i2);

        glm::vec3 worldV0 = glm::vec3(modelMatrix * glm::vec4(localV0, 1.0f));
        glm::vec3 worldV1 = glm::vec3(modelMatrix * glm::vec4(localV1, 1.0f));
        glm::vec3 worldV2 = glm::vec3(modelMatrix * glm::vec4(localV2, 1.0f));

        highlightVertices =
        {
            worldV0.x, worldV0.y, worldV0.z,
            worldV1.x, worldV1.y, worldV1.z,
            worldV2.x, worldV2.y, worldV2.z
        };
    }

    if (highlightVertices.empty())
    {
        return;
    }

    delete m_VBO;

    m_VAO.bind();

    m_VBO = new VertexBuffer(
        highlightVertices.data(),
        static_cast<unsigned int>(highlightVertices.size() * sizeof(float))
    );

    m_VBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    m_VAO.unbind();

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());
    m_Shader.setMat4("u_Model", glm::mat4(1.0f));

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    m_VAO.bind();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(highlightVertices.size() / 3));

    glDisable(GL_POLYGON_OFFSET_FILL);
}