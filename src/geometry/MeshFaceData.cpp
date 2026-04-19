#include "geometry/MeshFaceData.h"

void MeshFaceData::setLogicalFaces(
    const std::vector<LogicalFace>& logicalFaces,
    unsigned int triangleCount
)
{
    m_LogicalFaces = logicalFaces;
    m_TriangleToLogicalFace.assign(triangleCount, -1);

    for (unsigned int logicalFaceIndex = 0;
        logicalFaceIndex < m_LogicalFaces.size();
        ++logicalFaceIndex)
    {
        const std::vector<unsigned int>& triangleIndices =
            m_LogicalFaces[logicalFaceIndex].getTriangleIndices();

        for (unsigned int triangleIndex : triangleIndices)
        {
            if (triangleIndex < m_TriangleToLogicalFace.size())
            {
                m_TriangleToLogicalFace[triangleIndex] = static_cast<int>(logicalFaceIndex);
            }
        }
    }
}

void MeshFaceData::clear()
{
    m_LogicalFaces.clear();
    m_TriangleToLogicalFace.clear();
}

const std::vector<LogicalFace>& MeshFaceData::getLogicalFaces() const
{
    return m_LogicalFaces;
}

const LogicalFace* MeshFaceData::getLogicalFace(unsigned int logicalFaceIndex) const
{
    if (logicalFaceIndex >= m_LogicalFaces.size())
    {
        return nullptr;
    }

    return &m_LogicalFaces[logicalFaceIndex];
}

int MeshFaceData::getLogicalFaceIndexFromTriangle(unsigned int triangleIndex) const
{
    if (triangleIndex >= m_TriangleToLogicalFace.size())
    {
        return -1;
    }

    return m_TriangleToLogicalFace[triangleIndex];
}

bool MeshFaceData::hasLogicalFaces() const
{
    return !m_LogicalFaces.empty();
}