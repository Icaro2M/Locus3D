#include "Renderer.h"
#include <glad/glad.h>

void Renderer::clear() const
{
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Renderer::draw(const VertexArray& vao, const Shader& shader) const
{
    shader.use();
    vao.bind();

    glDrawArrays(GL_TRIANGLES, 0, 3);
}