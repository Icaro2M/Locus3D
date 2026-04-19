#include "FaceGeometry.h"

FaceGeometry::FaceGeometry()
    : m_Object(nullptr),
    m_FaceIndex(-1),
    m_LocalNormal(0.0f, 0.0f, 0.0f),
    m_LocalCenter(0.0f, 0.0f, 0.0f)
{
}

void FaceGeometry::setObject(SceneObject* object)
{
    m_Object = object;
}

void FaceGeometry::setFaceIndex(int faceIndex)
{
    m_FaceIndex = faceIndex;
}

void FaceGeometry::setTriangleIndices(const std::vector<unsigned int>& triangleIndices)
{
    m_TriangleIndices = triangleIndices;
}

void FaceGeometry::setBoundaryVertexIndices(const std::vector<unsigned int>& boundaryVertexIndices)
{
    m_BoundaryVertexIndices = boundaryVertexIndices;
}

void FaceGeometry::setLocalBoundaryVertices(const std::vector<glm::vec3>& localBoundaryVertices)
{
    m_LocalBoundaryVertices = localBoundaryVertices;
}

void FaceGeometry::setLocalNormal(const glm::vec3& normal)
{
    m_LocalNormal = normal;
}

void FaceGeometry::setLocalCenter(const glm::vec3& center)
{
    m_LocalCenter = center;
}

SceneObject* FaceGeometry::getObject() const
{
    return m_Object;
}

int FaceGeometry::getFaceIndex() const
{
    return m_FaceIndex;
}

const std::vector<unsigned int>& FaceGeometry::getTriangleIndices() const
{
    return m_TriangleIndices;
}

const std::vector<unsigned int>& FaceGeometry::getBoundaryVertexIndices() const
{
    return m_BoundaryVertexIndices;
}

const std::vector<glm::vec3>& FaceGeometry::getLocalBoundaryVertices() const
{
    return m_LocalBoundaryVertices;
}

const glm::vec3& FaceGeometry::getLocalNormal() const
{
    return m_LocalNormal;
}

const glm::vec3& FaceGeometry::getLocalCenter() const
{
    return m_LocalCenter;
}

bool FaceGeometry::isValid() const
{
    return m_Object != nullptr
        && m_FaceIndex >= 0
        && !m_TriangleIndices.empty()
        && m_LocalBoundaryVertices.size() >= 3;
}

void FaceGeometry::clear()
{
    m_Object = nullptr;
    m_FaceIndex = -1;

    m_TriangleIndices.clear();
    m_BoundaryVertexIndices.clear();
    m_LocalBoundaryVertices.clear();

    m_LocalNormal = glm::vec3(0.0f, 0.0f, 0.0f);
    m_LocalCenter = glm::vec3(0.0f, 0.0f, 0.0f);
}