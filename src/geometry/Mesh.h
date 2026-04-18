#pragma once

#include <vector>

#include "graphics/buffers/VertexArray.h"
#include "graphics/buffers/VertexBuffer.h"
#include "graphics/buffers/IndexBuffer.h"

#include "geometry/MeshFaceData.h"

#include <glm/glm/glm.hpp>

class Mesh
{
private:
    VertexArray m_VAO;
    VertexBuffer* m_VBO;
    IndexBuffer* m_IBO;

    std::vector<float> m_Vertices;
    std::vector<unsigned int> m_Indices;

    MeshFaceData m_FaceData;

public:
    Mesh(
        const float* vertices,
        unsigned int vertexBufferSize,
        const unsigned int* indices,
        unsigned int indexCount
    );

    ~Mesh();

    void bind() const;
    void unbind() const;

    const IndexBuffer& getIndexBuffer() const;

    const std::vector<float>& getVertices() const;
    const std::vector<unsigned int>& getIndices() const;

    glm::vec3 getVertexPosition(unsigned int vertexIndex) const;

    bool getTriangleVertexIndices(
        unsigned int triangleIndex,
        unsigned int& outI0,
        unsigned int& outI1,
        unsigned int& outI2
    ) const;

    void updateGeometry(
        const std::vector<float>& vertices,
        const std::vector<unsigned int>& indices
    );

    void setLogicalFaces(const std::vector<LogicalFace>& logicalFaces);
    void clearLogicalFaces();

    const std::vector<LogicalFace>& getLogicalFaces() const;
    const LogicalFace* getLogicalFace(unsigned int logicalFaceIndex) const;
    const LogicalFace* getLogicalFaceFromTriangle(unsigned int triangleIndex) const;
    int getLogicalFaceIndexFromTriangle(unsigned int triangleIndex) const;
    bool hasLogicalFaces() const;
};