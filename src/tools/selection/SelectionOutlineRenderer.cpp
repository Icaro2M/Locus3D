#include "SelectionOutlineRenderer.h"

#include "../../resources/AssetPaths.h"
#include "../../geometry/Mesh.h"
#include "../../geometry/LogicalFace.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include <vector>

SelectionOutlineRenderer::SelectionOutlineRenderer()
    : m_Shader(
        AssetPaths::shader("helpers/selection/selectionOutlineVertex.glsl"),
        AssetPaths::shader("helpers/selection/selectionOutlineFragment.glsl")
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

    glm::mat4 outlineModel = selectedObject.getTransform().getModelMatrix();
    outlineModel = glm::scale(outlineModel, glm::vec3(1.02f, 1.02f, 1.02f));

    if (!mesh.hasLogicalFaces())
    {
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

    const std::vector<LogicalFace>& logicalFaces = mesh.getLogicalFaces();
    std::vector<float> lineVertices;

    for (const LogicalFace& logicalFace : logicalFaces)
    {
        const std::vector<unsigned int>& boundary =
            logicalFace.getBoundaryVertexIndices();

        if (boundary.size() < 2)
        {
            continue;
        }

        for (unsigned int i = 0; i < boundary.size(); ++i)
        {
            unsigned int next = (i + 1) % static_cast<unsigned int>(boundary.size());

            glm::vec3 localV0 = mesh.getVertexPosition(boundary[i]);
            glm::vec3 localV1 = mesh.getVertexPosition(boundary[next]);

            lineVertices.push_back(localV0.x);
            lineVertices.push_back(localV0.y);
            lineVertices.push_back(localV0.z);

            lineVertices.push_back(localV1.x);
            lineVertices.push_back(localV1.y);
            lineVertices.push_back(localV1.z);
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

    m_Shader.setMat4("u_Model", outlineModel);

    glLineWidth(1.0f);

    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);

    m_VAO.bind();
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size() / 3));

    glDisable(GL_POLYGON_OFFSET_LINE);
}