#include "graphics/Renderer.h"
#include "geometry/Mesh.h"
#include "graphics/Shader.h"
#include <glad/glad.h>

void Renderer::clear() const
{
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::draw(const Mesh& mesh, const Shader& shader) const
{
    shader.use();
    mesh.bind();

    glDrawElements(GL_TRIANGLES, mesh.getIndexBuffer().getCount(), GL_UNSIGNED_INT, nullptr);
}