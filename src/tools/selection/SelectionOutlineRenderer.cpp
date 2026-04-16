#include "SelectionOutlineRenderer.h"

#include "../../resources/AssetPaths.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

SelectionOutlineRenderer::SelectionOutlineRenderer()
    : m_Shader(
        AssetPaths::shader("helpers/selection/selectionOutlineVertex.glsl"),
        AssetPaths::shader("helpers/selection/selectionOutlineFragment.glsl")
    )
{
}

void SelectionOutlineRenderer::render(const SceneObject& selectedObject, const Camera& camera)
{
    m_Shader.use();

    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

    glm::mat4 outlineModel = selectedObject.getTransform().getModelMatrix();
    outlineModel = glm::scale(outlineModel, glm::vec3(1.02f, 1.02f, 1.02f));

    m_Shader.setMat4("u_Model", outlineModel);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    selectedObject.getMesh().bind();
    glDrawElements(
        GL_TRIANGLES,
        selectedObject.getMesh().getIndexBuffer().getCount(),
        GL_UNSIGNED_INT,
        nullptr
    );

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}