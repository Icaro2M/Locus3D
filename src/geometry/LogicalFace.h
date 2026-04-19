#pragma once

#include <vector>

class LogicalFace
{
private:
    std::vector<unsigned int> m_TriangleIndices;
    std::vector<unsigned int> m_BoundaryVertexIndices;

public:
    LogicalFace() = default;

    LogicalFace(
        const std::vector<unsigned int>& triangleIndices,
        const std::vector<unsigned int>& boundaryVertexIndices
    );

    void setTriangleIndices(const std::vector<unsigned int>& triangleIndices);
    void setBoundaryVertexIndices(const std::vector<unsigned int>& boundaryVertexIndices);

    const std::vector<unsigned int>& getTriangleIndices() const;
    const std::vector<unsigned int>& getBoundaryVertexIndices() const;

    bool containsTriangle(unsigned int triangleIndex) const;
    bool isValid() const;
};