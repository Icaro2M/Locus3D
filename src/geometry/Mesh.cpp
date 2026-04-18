#include "geometry/Mesh.h"

#include <glad/glad.h>

Mesh::Mesh(
    const float* vertices,
    unsigned int vertexBufferSize,
    const unsigned int* indices,
    unsigned int indexCount
)
{
    unsigned int floatCount = vertexBufferSize / sizeof(float);

    m_Vertices.assign(vertices, vertices + floatCount);
    m_Indices.assign(indices, indices + indexCount);

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

const std::vector<float>& Mesh::getVertices() const
{
    return m_Vertices;
}

const std::vector<unsigned int>& Mesh::getIndices() const
{
    return m_Indices;
}

glm::vec3 Mesh::getVertexPosition(unsigned int vertexIndex) const
{
    unsigned int baseIndex = vertexIndex * 6;

    float vertexX = m_Vertices[baseIndex];
    float vertexY = m_Vertices[baseIndex + 1];
    float vertexZ = m_Vertices[baseIndex + 2];

    return glm::vec3(vertexX, vertexY, vertexZ);
}

bool Mesh::getTriangleVertexIndices(
    unsigned int triangleIndex,
    unsigned int& outI0,
    unsigned int& outI1,
    unsigned int& outI2
) const
{
    unsigned int base = triangleIndex * 3;

    if (base + 2 >= m_Indices.size())
    {
        outI0 = 0;
        outI1 = 0;
        outI2 = 0;
        return false;
    }

    outI0 = m_Indices[base];
    outI1 = m_Indices[base + 1];
    outI2 = m_Indices[base + 2];

    return true;
}

void Mesh::updateGeometry(
    const std::vector<float>& vertices,
    const std::vector<unsigned int>& indices
)
{
    m_Vertices = vertices;
    m_Indices = indices;

    delete m_VBO;
    delete m_IBO;

    unsigned int vertexBufferSize = static_cast<unsigned int>(m_Vertices.size() * sizeof(float));
    unsigned int indexCount = static_cast<unsigned int>(m_Indices.size());

    m_VBO = new VertexBuffer(m_Vertices.data(), vertexBufferSize);
    m_IBO = new IndexBuffer(m_Indices.data(), indexCount);

    m_VAO.bind();
    m_VBO->bind();
    m_IBO->bind();

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_VAO.unbind();

    clearLogicalFaces();
}

void Mesh::setLogicalFaces(const std::vector<LogicalFace>& logicalFaces)
{
    unsigned int triangleCount = static_cast<unsigned int>(m_Indices.size() / 3);
    m_FaceData.setLogicalFaces(logicalFaces, triangleCount);
}

void Mesh::clearLogicalFaces()
{
    m_FaceData.clear();
}

const std::vector<LogicalFace>& Mesh::getLogicalFaces() const
{
    return m_FaceData.getLogicalFaces();
}

const LogicalFace* Mesh::getLogicalFace(unsigned int logicalFaceIndex) const
{
    return m_FaceData.getLogicalFace(logicalFaceIndex);
}

const LogicalFace* Mesh::getLogicalFaceFromTriangle(unsigned int triangleIndex) const
{
    int logicalFaceIndex = m_FaceData.getLogicalFaceIndexFromTriangle(triangleIndex);

    if (logicalFaceIndex < 0)
    {
        return nullptr;
    }

    return m_FaceData.getLogicalFace(static_cast<unsigned int>(logicalFaceIndex));
}

int Mesh::getLogicalFaceIndexFromTriangle(unsigned int triangleIndex) const
{
    return m_FaceData.getLogicalFaceIndexFromTriangle(triangleIndex);
}

bool Mesh::hasLogicalFaces() const
{
    return m_FaceData.hasLogicalFaces();
}