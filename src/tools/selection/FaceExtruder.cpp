#include "FaceExtruder.h"

#include "../../geometry/LogicalFace.h"
#include "../../geometry/Mesh.h"

#include <glm/glm/glm.hpp>

#include <vector>

namespace
{
    void appendVertex(
        std::vector<float>& vertices,
        const glm::vec3& position,
        const glm::vec3& normal
    )
    {
        vertices.push_back(position.x);
        vertices.push_back(position.y);
        vertices.push_back(position.z);
        vertices.push_back(normal.x);
        vertices.push_back(normal.y);
        vertices.push_back(normal.z);
    }

    unsigned int appendTriangle(
        std::vector<unsigned int>& indices,
        unsigned int i0,
        unsigned int i1,
        unsigned int i2
    )
    {
        unsigned int triangleIndex = static_cast<unsigned int>(indices.size() / 3);

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        return triangleIndex;
    }

    glm::vec3 computeNormal(
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2
    )
    {
        glm::vec3 normal = glm::cross(v1 - v0, v2 - v0);

        if (glm::length(normal) <= 0.00001f)
        {
            return glm::vec3(0.0f, 0.0f, 1.0f);
        }

        return glm::normalize(normal);
    }

    LogicalFace buildRemappedLogicalFace(
        const LogicalFace& originalFace,
        const std::vector<int>& oldTriangleToNewTriangle
    )
    {
        std::vector<unsigned int> remappedTriangleIndices;

        for (unsigned int oldTriangleIndex : originalFace.getTriangleIndices())
        {
            if (oldTriangleIndex < oldTriangleToNewTriangle.size())
            {
                int remapped = oldTriangleToNewTriangle[oldTriangleIndex];
                if (remapped >= 0)
                {
                    remappedTriangleIndices.push_back(static_cast<unsigned int>(remapped));
                }
            }
        }

        return LogicalFace(
            remappedTriangleIndices,
            originalFace.getBoundaryVertexIndices()
        );
    }
}

bool FaceExtruder::extrude(FaceSelection& selection, float distance)
{
    if (!selection.isValid())
    {
        return false;
    }

    FaceGeometry geometry = m_FaceGeometryBuilder.build(selection);
    if (!geometry.isValid())
    {
        return false;
    }

    SceneObject* object = geometry.getObject();
    if (object == nullptr)
    {
        return false;
    }

    Mesh& mesh = object->getMesh();

    if (!mesh.hasLogicalFaces())
    {
        return false;
    }

    const std::vector<glm::vec3>& boundaryVertices = geometry.getLocalBoundaryVertices();
    const std::vector<unsigned int>& boundaryVertexIndices = geometry.getBoundaryVertexIndices();
    const std::vector<unsigned int>& selectedTriangleIndices = geometry.getTriangleIndices();

    if (boundaryVertices.size() < 3 || boundaryVertexIndices.size() < 3)
    {
        return false;
    }

    glm::vec3 faceNormal = geometry.getLocalNormal();
    if (glm::length(faceNormal) <= 0.00001f)
    {
        return false;
    }

    faceNormal = glm::normalize(faceNormal);

    std::vector<float> newVertices = mesh.getVertices();
    std::vector<unsigned int> newIndices;
    newIndices.reserve(mesh.getIndices().size() + boundaryVertices.size() * 18);

    const std::vector<unsigned int>& oldIndices = mesh.getIndices();
    const unsigned int oldTriangleCount = static_cast<unsigned int>(oldIndices.size() / 3);

    std::vector<bool> removeTriangle(oldTriangleCount, false);
    for (unsigned int triangleIndex : selectedTriangleIndices)
    {
        if (triangleIndex < removeTriangle.size())
        {
            removeTriangle[triangleIndex] = true;
        }
    }

    std::vector<int> oldTriangleToNewTriangle(oldTriangleCount, -1);

    for (unsigned int oldTriangleIndex = 0; oldTriangleIndex < oldTriangleCount; ++oldTriangleIndex)
    {
        if (removeTriangle[oldTriangleIndex])
        {
            continue;
        }

        unsigned int base = oldTriangleIndex * 3;

        unsigned int newTriangleIndex = static_cast<unsigned int>(newIndices.size() / 3);

        newIndices.push_back(oldIndices[base]);
        newIndices.push_back(oldIndices[base + 1]);
        newIndices.push_back(oldIndices[base + 2]);

        oldTriangleToNewTriangle[oldTriangleIndex] = static_cast<int>(newTriangleIndex);
    }

    std::vector<LogicalFace> rebuiltLogicalFaces;
    const std::vector<LogicalFace>& oldLogicalFaces = mesh.getLogicalFaces();

    for (unsigned int logicalFaceIndex = 0; logicalFaceIndex < oldLogicalFaces.size(); ++logicalFaceIndex)
    {
        if (static_cast<int>(logicalFaceIndex) == geometry.getFaceIndex())
        {
            continue;
        }

        LogicalFace remappedFace =
            buildRemappedLogicalFace(oldLogicalFaces[logicalFaceIndex], oldTriangleToNewTriangle);

        if (remappedFace.isValid())
        {
            rebuiltLogicalFaces.push_back(remappedFace);
        }
    }

    std::vector<unsigned int> topBoundaryVertexIndices;
    topBoundaryVertexIndices.reserve(boundaryVertices.size());

    for (const glm::vec3& baseVertex : boundaryVertices)
    {
        glm::vec3 extrudedVertex = baseVertex + faceNormal * distance;
        unsigned int newVertexIndex = static_cast<unsigned int>(newVertices.size() / 6);

        appendVertex(newVertices, extrudedVertex, faceNormal);
        topBoundaryVertexIndices.push_back(newVertexIndex);
    }

    std::vector<unsigned int> topTriangleIndices;
    topTriangleIndices.reserve(boundaryVertices.size());

    glm::vec3 topCenterPosition = geometry.getLocalCenter() + faceNormal * distance;
    unsigned int topCenterIndex = static_cast<unsigned int>(newVertices.size() / 6);

    appendVertex(newVertices, topCenterPosition, faceNormal);

    for (unsigned int i = 0; i < topBoundaryVertexIndices.size(); ++i)
    {
        unsigned int next = (i + 1) % static_cast<unsigned int>(topBoundaryVertexIndices.size());

        unsigned int triangleIndex = appendTriangle(
            newIndices,
            topCenterIndex,
            topBoundaryVertexIndices[i],
            topBoundaryVertexIndices[next]
        );

        topTriangleIndices.push_back(triangleIndex);
    }

    unsigned int newTopLogicalFaceIndex = static_cast<unsigned int>(rebuiltLogicalFaces.size());

    rebuiltLogicalFaces.push_back(
        LogicalFace(topTriangleIndices, topBoundaryVertexIndices)
    );

    const unsigned int boundaryCount = static_cast<unsigned int>(boundaryVertices.size());

    for (unsigned int i = 0; i < boundaryCount; ++i)
    {
        unsigned int next = (i + 1) % boundaryCount;

        const glm::vec3& baseV0 = boundaryVertices[i];
        const glm::vec3& baseV1 = boundaryVertices[next];
        const glm::vec3 topV0 = baseV0 + faceNormal * distance;
        const glm::vec3 topV1 = baseV1 + faceNormal * distance;

        glm::vec3 sideNormal = computeNormal(baseV0, baseV1, topV1);

        unsigned int sideI0 = static_cast<unsigned int>(newVertices.size() / 6);
        appendVertex(newVertices, baseV0, sideNormal);

        unsigned int sideI1 = static_cast<unsigned int>(newVertices.size() / 6);
        appendVertex(newVertices, baseV1, sideNormal);

        unsigned int sideI2 = static_cast<unsigned int>(newVertices.size() / 6);
        appendVertex(newVertices, topV1, sideNormal);

        unsigned int sideI3 = static_cast<unsigned int>(newVertices.size() / 6);
        appendVertex(newVertices, topV0, sideNormal);

        unsigned int tri0 = appendTriangle(newIndices, sideI0, sideI1, sideI2);
        unsigned int tri1 = appendTriangle(newIndices, sideI0, sideI2, sideI3);

        rebuiltLogicalFaces.push_back(
            LogicalFace(
                { tri0, tri1 },
                { sideI0, sideI1, sideI2, sideI3 }
            )
        );
    }

    mesh.updateGeometry(newVertices, newIndices);
    mesh.setLogicalFaces(rebuiltLogicalFaces);

    selection.set(object, static_cast<int>(newTopLogicalFaceIndex));

    return true;
}