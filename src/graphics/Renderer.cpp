#include "graphics/Renderer.h"
#include "geometry/Mesh.h"
#include "graphics/Shader.h"
#include <glad/glad.h>

void Renderer::clear() const
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::draw(const Mesh& mesh, const Shader& shader) const
{
    mesh.bind();

    glDrawElements(GL_TRIANGLES, mesh.getIndexBuffer().getCount(), GL_UNSIGNED_INT, nullptr);
}

void Renderer::renderScene(const Scene& scene, const Camera& camera, Shader& shader) const
{
    shader.use();
    shader.setMat4("u_View", camera.getViewMatrix());
    shader.setMat4("u_Projection", camera.getProjectionMatrix());

    for (const SceneObject* object : scene.getObjects())
    {
        shader.setMat4("u_Model", object->getTransform().getModelMatrix());
        draw(object->getMesh(), shader);
    }
}