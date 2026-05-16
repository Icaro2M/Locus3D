#include "FaceGeometryBuilder.h"

#include "../../geometry/LogicalFace.h"
#include "../../geometry/Mesh.h"

#include <algorithm>
#include <glm/glm/glm.hpp>

namespace
{
    glm::vec3 computeNormalFromLogicalFace(
        const Mesh& mesh,
        const LogicalFace& logicalFace
    )
    {
        const std::vector<unsigned int>& triangleIndices = logicalFace.getTriangleIndices();

        for (unsigned int triangleIndex : triangleIndices)
        {
            unsigned int i0 = 0;
            unsigned int i1 = 0;
            unsigned int i2 = 0;

            if (!mesh.getTriangleVertexIndices(triangleIndex, i0, i1, i2))
            {
                continue;
            }

            const glm::vec3 v0 = mesh.getVertexPosition(i0);
            const glm::vec3 v1 = mesh.getVertexPosition(i1);
            const glm::vec3 v2 = mesh.getVertexPosition(i2);

            const glm::vec3 edge1 = v1 - v0;
            const glm::vec3 edge2 = v2 - v0;
            const glm::vec3 normal = glm::cross(edge1, edge2);

            if (glm::length(normal) > 0.00001f)
            {
                return glm::normalize(normal);
            }
        }

        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    glm::vec3 computeCenterFromBoundary(
        const std::vector<glm::vec3>& boundaryVertices
    )
    {
        if (boundaryVertices.empty())
        {
            return glm::vec3(0.0f, 0.0f, 0.0f);
        }

        glm::vec3 center(0.0f, 0.0f, 0.0f);

        for (const glm::vec3& vertex : boundaryVertices)
        {
            center += vertex;
        }

        return center / static_cast<float>(boundaryVertices.size());
    }

    glm::vec3 computeNormalFromBoundary(
        const std::vector<glm::vec3>& boundaryVertices
    )
    {
        if (boundaryVertices.size() < 3)
        {
            return glm::vec3(0.0f, 0.0f, 0.0f);
        }

        glm::vec3 normal(0.0f, 0.0f, 0.0f);

        for (size_t i = 0; i < boundaryVertices.size(); ++i)
        {
            const glm::vec3& current = boundaryVertices[i];
            const glm::vec3& next = boundaryVertices[(i + 1) % boundaryVertices.size()];

            normal.x += (current.y - next.y) * (current.z + next.z);
            normal.y += (current.z - next.z) * (current.x + next.x);
            normal.z += (current.x - next.x) * (current.y + next.y);
        }

        if (glm::length(normal) <= 0.00001f)
        {
            return glm::vec3(0.0f, 0.0f, 0.0f);
        }

        return glm::normalize(normal);
    }
}

FaceGeometry FaceGeometryBuilder::build(const FaceSelection& selection) const
{
    FaceGeometry geometry;

    if (!selection.isValid())
    {
        return geometry;
    }

    SceneObject* object = selection.getObject();

    if (object == nullptr)
    {
        return geometry;
    }

    const Mesh& mesh = object->getMesh();

    if (!mesh.hasLogicalFaces())
    {
        return geometry;
    }

    const LogicalFace* logicalFace = mesh.getLogicalFace(
        static_cast<unsigned int>(selection.getFaceIndex())
    );

    if (logicalFace == nullptr || !logicalFace->isValid())
    {
        return geometry;
    }

    std::vector<unsigned int> boundaryVertexIndices = logicalFace->getBoundaryVertexIndices();

    std::vector<glm::vec3> localBoundaryVertices;
    localBoundaryVertices.reserve(boundaryVertexIndices.size());

    for (unsigned int vertexIndex : boundaryVertexIndices)
    {
        localBoundaryVertices.push_back(mesh.getVertexPosition(vertexIndex));
    }

    if (localBoundaryVertices.size() < 3)
    {
        return geometry;
    }

    const glm::vec3 localNormal = computeNormalFromLogicalFace(mesh, *logicalFace);

    if (glm::length(localNormal) <= 0.00001f)
    {
        return geometry;
    }

    const glm::vec3 boundaryNormal = computeNormalFromBoundary(localBoundaryVertices);

    if (glm::length(boundaryNormal) > 0.00001f &&
        glm::dot(boundaryNormal, localNormal) < 0.0f)
    {
        std::reverse(boundaryVertexIndices.begin(), boundaryVertexIndices.end());
        std::reverse(localBoundaryVertices.begin(), localBoundaryVertices.end());
    }

    const glm::vec3 localCenter = computeCenterFromBoundary(localBoundaryVertices);

    geometry.setObject(object);
    geometry.setFaceIndex(selection.getFaceIndex());
    geometry.setTriangleIndices(logicalFace->getTriangleIndices());
    geometry.setBoundaryVertexIndices(boundaryVertexIndices);
    geometry.setLocalBoundaryVertices(localBoundaryVertices);
    geometry.setLocalNormal(localNormal);
    geometry.setLocalCenter(localCenter);

    return geometry;
}