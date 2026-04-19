#pragma once

#include <vector>

#include "geometry/LogicalFace.h"

class MeshFaceData
{
private:
    std::vector<LogicalFace> m_LogicalFaces;
    std::vector<int> m_TriangleToLogicalFace;

public:
    MeshFaceData() = default;

    void setLogicalFaces(
        const std::vector<LogicalFace>& logicalFaces,
        unsigned int triangleCount
    );

    void clear();

    const std::vector<LogicalFace>& getLogicalFaces() const;
    const LogicalFace* getLogicalFace(unsigned int logicalFaceIndex) const;
    int getLogicalFaceIndexFromTriangle(unsigned int triangleIndex) const;

    bool hasLogicalFaces() const;
};