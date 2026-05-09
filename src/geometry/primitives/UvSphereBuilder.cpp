#include "UvSphereBuilder.h"

#include <cmath>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/constants.hpp>

namespace
{
    void addVertex(
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

    void addTriangle(
        std::vector<unsigned int>& indices,
        unsigned int a,
        unsigned int b,
        unsigned int c
    )
    {
        indices.push_back(a);
        indices.push_back(b);
        indices.push_back(c);
    }
}

UvSphereBuilder::Result UvSphereBuilder::build(
    int segments,
    int rings,
    float radiusX,
    float radiusY,
    float radiusZ
)
{
    Result result;

    if (segments < 3)
    {
        segments = 3;
    }

    if (rings < 2)
    {
        rings = 2;
    }

    if (radiusX <= 0.0f)
    {
        radiusX = 0.5f;
    }

    if (radiusY <= 0.0f)
    {
        radiusY = 0.5f;
    }

    if (radiusZ <= 0.0f)
    {
        radiusZ = 0.5f;
    }

    for (int ring = 0; ring <= rings; ring++)
    {
        float v = static_cast<float>(ring) / static_cast<float>(rings);
        float phi = glm::pi<float>() * v;

        float yUnit = std::cos(phi);
        float ringRadius = std::sin(phi);

        for (int segment = 0; segment <= segments; segment++)
        {
            float u = static_cast<float>(segment) / static_cast<float>(segments);
            float theta = glm::two_pi<float>() * u;

            float xUnit = std::cos(theta) * ringRadius;
            float zUnit = std::sin(theta) * ringRadius;

            glm::vec3 unitPosition(xUnit, yUnit, zUnit);

            glm::vec3 position(
                xUnit * radiusX,
                yUnit * radiusY,
                zUnit * radiusZ
            );

            glm::vec3 normal(
                unitPosition.x / radiusX,
                unitPosition.y / radiusY,
                unitPosition.z / radiusZ
            );

            if (glm::length(normal) > 0.00001f)
            {
                normal = glm::normalize(normal);
            }
            else
            {
                normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            addVertex(result.vertices, position, normal);
        }
    }

    int rowSize = segments + 1;
    unsigned int currentTriangleIndex = 0;

    for (int ring = 0; ring < rings; ring++)
    {
        for (int segment = 0; segment < segments; segment++)
        {
            unsigned int a = static_cast<unsigned int>(ring * rowSize + segment);
            unsigned int b = static_cast<unsigned int>((ring + 1) * rowSize + segment);
            unsigned int c = static_cast<unsigned int>((ring + 1) * rowSize + segment + 1);
            unsigned int d = static_cast<unsigned int>(ring * rowSize + segment + 1);

            addTriangle(result.indices, a, c, b);
            addTriangle(result.indices, a, d, c);

            result.logicalFaces.push_back(
                LogicalFace(
                    { currentTriangleIndex, currentTriangleIndex + 1 },
                    { a, d, c, b }
                )
            );

            currentTriangleIndex += 2;
        }
    }

    return result;
}