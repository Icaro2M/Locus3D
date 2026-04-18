#include "FaceHighlightRenderer.h"

#include <glm/glm/glm.hpp>

#include <vector>

FaceHighlightRenderer::FaceHighlightRenderer()
    : m_Shader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\selection\\faceSelectionVertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\selection\\faceSelectionFragment.glsl"
    ),
    m_VBO(nullptr)
{
    // Buffer inicial pequeno; será atualizado dinamicamente no render.
    float vertices[] =
    {
        0.0f, 0.0f, 0.0f
    };

    m_VAO.bind();

    m_VBO = new VertexBuffer(vertices, sizeof(vertices));
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

void FaceHighlightRenderer::render(const FaceLogicalGeometry& logicalFace, const Camera& camera)
{
    if (!logicalFace.isValid())
    {
        return;
    }

    SceneObject* object = logicalFace.getObject();
    if (object == nullptr)
    {
        return;
    }

    const std::vector<glm::vec3>& localBoundary = logicalFace.getLocalBoundaryVertices();
    if (localBoundary.size() < 3)
    {
        return;
    }

    glm::mat4 modelMatrix = object->getTransform().getModelMatrix();

    std::vector<float> highlightVertices;
    highlightVertices.reserve(localBoundary.size() * 3);

    for (const glm::vec3& localVertex : localBoundary)
    {
        glm::vec3 worldVertex = glm::vec3(modelMatrix * glm::vec4(localVertex, 1.0f));

        highlightVertices.push_back(worldVertex.x);
        highlightVertices.push_back(worldVertex.y);
        highlightVertices.push_back(worldVertex.z);
    }

    m_VBO->bind();
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(highlightVertices.size() * sizeof(float)),
        highlightVertices.data(),
        GL_DYNAMIC_DRAW
    );

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());
    m_Shader.setMat4("u_Model", glm::mat4(1.0f));

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    m_VAO.bind();
    glDrawArrays(GL_TRIANGLE_FAN, 0, static_cast<GLsizei>(localBoundary.size()));

    glDisable(GL_POLYGON_OFFSET_FILL);
}