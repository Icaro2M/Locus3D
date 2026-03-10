#include "Mesh.h"
#include <glad/glad.h>

Mesh::Mesh(const float* vertices, unsigned int size)
{
    vao.bind();

    vbo = new VertexBuffer(vertices, size);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    vertexCount = size / (3 * sizeof(float));
}

Mesh::~Mesh()
{
    delete vbo;
}

const VertexArray& Mesh::getVAO() const
{
    return vao;
}

unsigned int Mesh::getVertexCount() const
{
    return vertexCount;
}