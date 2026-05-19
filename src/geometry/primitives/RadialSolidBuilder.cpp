#include "RadialSolidBuilder.h"

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

    glm::vec3 computeNormal(
        const glm::vec3& a,
        const glm::vec3& b,
        const glm::vec3& c
    )
    {
        glm::vec3 normal = glm::cross(b - a, c - a);

        if (glm::length(normal) <= 0.00001f)
        {
            return glm::vec3(0.0f, 0.0f, 1.0f);
        }

        return glm::normalize(normal);
    }
}

RadialSolidBuilder::Result RadialSolidBuilder::build(
    int sides,
    float height,
    float bottomRadius,
    float topRadius,
    bool capBottom,
    bool capTop
)
{
    Result result;

    if (sides < 3)
    {
        sides = 3;
    }

    if (height <= 0.0f)
    {
        height = 1.0f;
    }

    if (bottomRadius < 0.0f)
    {
        bottomRadius = 0.0f;
    }

    if (topRadius < 0.0f)
    {
        topRadius = 0.0f;
    }

    float halfHeight = height * 0.5f;
    float bottomY = -halfHeight;
    float topY = halfHeight;

    unsigned int currentTriangleIndex = 0;

    for (int i = 0; i < sides; i++)
    {
        int next = (i + 1) % sides;

        float angle0 = (2.0f * glm::pi<float>() * static_cast<float>(i)) / static_cast<float>(sides);
        float angle1 = (2.0f * glm::pi<float>() * static_cast<float>(next)) / static_cast<float>(sides);

        glm::vec3 bottom0(
            std::cos(angle0) * bottomRadius,
            bottomY,
            std::sin(angle0) * bottomRadius
        );

        glm::vec3 bottom1(
            std::cos(angle1) * bottomRadius,
            bottomY,
            std::sin(angle1) * bottomRadius
        );

        glm::vec3 top0(
            std::cos(angle0) * topRadius,
            topY,
            std::sin(angle0) * topRadius
        );

        glm::vec3 top1(
            std::cos(angle1) * topRadius,
            topY,
            std::sin(angle1) * topRadius
        );

        if (bottomRadius <= 0.00001f && topRadius > 0.00001f)
        {
            glm::vec3 normal = computeNormal(bottom0, top0, top1);

            unsigned int baseVertexIndex = static_cast<unsigned int>(result.vertices.size() / 6);

            addVertex(result.vertices, bottom0, normal);
            addVertex(result.vertices, top0, normal);
            addVertex(result.vertices, top1, normal);

            addTriangle(result.indices, baseVertexIndex + 0, baseVertexIndex + 1, baseVertexIndex + 2);

            result.logicalFaces.push_back(
                LogicalFace(
                    { currentTriangleIndex },
                    {
                    baseVertexIndex + 0,
                    baseVertexIndex + 1,
                    baseVertexIndex + 2
                    }
                )
            );

            currentTriangleIndex += 1;
        }
        else if (topRadius <= 0.00001f && bottomRadius > 0.00001f)
        {
            glm::vec3 normal = computeNormal(bottom0, top0, bottom1);

            unsigned int baseVertexIndex = static_cast<unsigned int>(result.vertices.size() / 6);

            addVertex(result.vertices, bottom0, normal);
            addVertex(result.vertices, top0, normal);
            addVertex(result.vertices, bottom1, normal);

            addTriangle(result.indices, baseVertexIndex + 0, baseVertexIndex + 1, baseVertexIndex + 2);

            result.logicalFaces.push_back(
                LogicalFace(
                    { currentTriangleIndex },
                    {
                    baseVertexIndex + 0,
                    baseVertexIndex + 1,
                    baseVertexIndex + 2
                    }
                )
            );

            currentTriangleIndex += 1;
        }
        else if (bottomRadius > 0.00001f || topRadius > 0.00001f)
        {
            glm::vec3 normalA = computeNormal(bottom0, top1, bottom1);
            glm::vec3 normalB = computeNormal(bottom0, top0, top1);
            glm::vec3 normal = normalA + normalB;

            if (glm::length(normal) > 0.00001f)
            {
                normal = glm::normalize(normal);
            }
            else
            {
                normal = normalA;
            }

            unsigned int baseVertexIndex = static_cast<unsigned int>(result.vertices.size() / 6);

            addVertex(result.vertices, bottom0, normal);
            addVertex(result.vertices, bottom1, normal);
            addVertex(result.vertices, top1, normal);
            addVertex(result.vertices, top0, normal);

            addTriangle(result.indices, baseVertexIndex + 0, baseVertexIndex + 2, baseVertexIndex + 1);
            addTriangle(result.indices, baseVertexIndex + 0, baseVertexIndex + 3, baseVertexIndex + 2);

            result.logicalFaces.push_back(
                LogicalFace(
                    { currentTriangleIndex, currentTriangleIndex + 1 },
                    {
                    baseVertexIndex + 0,
                    baseVertexIndex + 3,
                    baseVertexIndex + 2,
                    baseVertexIndex + 1
                    }
                )
            );

            currentTriangleIndex += 2;
        }
    }

    if (capBottom && bottomRadius > 0.0f)
    {
        std::vector<unsigned int> bottomTriangleIndices;
        std::vector<unsigned int> bottomBoundaryVertexIndices;

        unsigned int centerIndex = static_cast<unsigned int>(result.vertices.size() / 6);

        addVertex(
            result.vertices,
            glm::vec3(0.0f, bottomY, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f)
        );

        for (int i = 0; i < sides; i++)
        {
            int next = (i + 1) % sides;

            float angle0 = (2.0f * glm::pi<float>() * static_cast<float>(i)) / static_cast<float>(sides);
            float angle1 = (2.0f * glm::pi<float>() * static_cast<float>(next)) / static_cast<float>(sides);

            glm::vec3 p0(
                std::cos(angle0) * bottomRadius,
                bottomY,
                std::sin(angle0) * bottomRadius
            );

            glm::vec3 p1(
                std::cos(angle1) * bottomRadius,
                bottomY,
                std::sin(angle1) * bottomRadius
            );

            unsigned int ringIndex = static_cast<unsigned int>(result.vertices.size() / 6);

            addVertex(result.vertices, p0, glm::vec3(0.0f, -1.0f, 0.0f));
            addVertex(result.vertices, p1, glm::vec3(0.0f, -1.0f, 0.0f));

            addTriangle(result.indices, centerIndex, ringIndex + 0, ringIndex + 1);

            bottomTriangleIndices.push_back(currentTriangleIndex);
            bottomBoundaryVertexIndices.push_back(ringIndex + 0);

            currentTriangleIndex += 1;
        }

        result.logicalFaces.push_back(
            LogicalFace(bottomTriangleIndices, bottomBoundaryVertexIndices)
        );
    }

    if (capTop && topRadius > 0.0f)
    {
        std::vector<unsigned int> topTriangleIndices;
        std::vector<unsigned int> topBoundaryVertexIndices;

        unsigned int centerIndex = static_cast<unsigned int>(result.vertices.size() / 6);

        addVertex(
            result.vertices,
            glm::vec3(0.0f, topY, 0.0f),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        for (int i = 0; i < sides; i++)
        {
            int next = (i + 1) % sides;

            float angle0 = (2.0f * glm::pi<float>() * static_cast<float>(i)) / static_cast<float>(sides);
            float angle1 = (2.0f * glm::pi<float>() * static_cast<float>(next)) / static_cast<float>(sides);

            glm::vec3 p0(
                std::cos(angle0) * topRadius,
                topY,
                std::sin(angle0) * topRadius
            );

            glm::vec3 p1(
                std::cos(angle1) * topRadius,
                topY,
                std::sin(angle1) * topRadius
            );

            unsigned int ringIndex = static_cast<unsigned int>(result.vertices.size() / 6);

            addVertex(result.vertices, p0, glm::vec3(0.0f, 1.0f, 0.0f));
            addVertex(result.vertices, p1, glm::vec3(0.0f, 1.0f, 0.0f));

            addTriangle(result.indices, centerIndex, ringIndex + 1, ringIndex + 0);

            topTriangleIndices.push_back(currentTriangleIndex);
            topBoundaryVertexIndices.push_back(ringIndex + 0);

            currentTriangleIndex += 1;
        }

        result.logicalFaces.push_back(
            LogicalFace(topTriangleIndices, topBoundaryVertexIndices)
        );
    }

    return result;
}
