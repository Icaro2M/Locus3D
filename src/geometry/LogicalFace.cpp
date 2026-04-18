#include "geometry/LogicalFace.h"

#include <algorithm>

LogicalFace::LogicalFace(
    const std::vector<unsigned int>& triangleIndices,
    const std::vector<unsigned int>& boundaryVertexIndices
)
    : m_TriangleIndices(triangleIndices),
    m_BoundaryVertexIndices(boundaryVertexIndices)
{
}

void LogicalFace::setTriangleIndices(const std::vector<unsigned int>& triangleIndices)
{
    m_TriangleIndices = triangleIndices;
}

void LogicalFace::setBoundaryVertexIndices(const std::vector<unsigned int>& boundaryVertexIndices)
{
    m_BoundaryVertexIndices = boundaryVertexIndices;
}

const std::vector<unsigned int>& LogicalFace::getTriangleIndices() const
{
    return m_TriangleIndices;
}

const std::vector<unsigned int>& LogicalFace::getBoundaryVertexIndices() const
{
    return m_BoundaryVertexIndices;
}

bool LogicalFace::containsTriangle(unsigned int triangleIndex) const
{
    return std::find(
        m_TriangleIndices.begin(),
        m_TriangleIndices.end(),
        triangleIndex
    ) != m_TriangleIndices.end();
}

bool LogicalFace::isValid() const
{
    return !m_TriangleIndices.empty() && m_BoundaryVertexIndices.size() >= 3;
}