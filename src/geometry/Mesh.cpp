#include "geometry/Mesh.h"
#include <glad/glad.h>

Mesh::Mesh(const float* vertices, unsigned int vertexBufferSize,
    const unsigned int* indices, unsigned int indexCount)
{
    m_VBO = new VertexBuffer(vertices, vertexBufferSize);
    m_IBO = new IndexBuffer(indices, indexCount);

    m_VAO.bind();
    m_VBO->bind();
    m_IBO->bind();


    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_VAO.unbind();
}

Mesh::~Mesh()
{
    delete m_VBO;
    delete m_IBO;
}

void Mesh::bind() const
{
    m_VAO.bind();
    m_IBO->bind();
}

void Mesh::unbind() const
{
    m_VAO.unbind();
}

const IndexBuffer& Mesh::getIndexBuffer() const
{
    return *m_IBO;
}