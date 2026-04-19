#include "FaceSelector.h"
#include "Raycaster.h"

#include "../../geometry/Mesh.h"

#include <limits>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

namespace
{
    bool intersectRayTriangle(
        const Ray& ray,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& outT
    )
    {
        const float epsilon = 0.000001f;

        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;

        glm::vec3 h = glm::cross(ray.direction, edge2);
        float a = glm::dot(edge1, h);

        if (a > -epsilon && a < epsilon)
        {
            return false;
        }

        float f = 1.0f / a;
        glm::vec3 s = ray.origin - v0;
        float u = f * glm::dot(s, h);

        if (u < 0.0f || u > 1.0f)
        {
            return false;
        }

        glm::vec3 q = glm::cross(s, edge1);
        float v = f * glm::dot(ray.direction, q);

        if (v < 0.0f || u + v > 1.0f)
        {
            return false;
        }

        float t = f * glm::dot(edge2, q);

        if (t > epsilon)
        {
            outT = t;
            return true;
        }

        return false;
    }
}

int FaceSelector::selectFace(
    const SceneObject& selectedObject,
    GLFWwindow* window,
    const Camera& camera
) const
{
    Raycaster raycaster;
    Ray ray = raycaster.buildRayFromMouse(window, camera);

    const Mesh& mesh = selectedObject.getMesh();
    glm::mat4 modelMatrix = selectedObject.getTransform().getModelMatrix();

    float closestT = std::numeric_limits<float>::max();
    int selectedTriangleIndex = -1;

    unsigned int triangleCount = static_cast<unsigned int>(mesh.getIndices().size() / 3);

    for (unsigned int triangleIndex = 0; triangleIndex < triangleCount; triangleIndex++)
    {
        unsigned int i0 = 0;
        unsigned int i1 = 0;
        unsigned int i2 = 0;

        if (!mesh.getTriangleVertexIndices(triangleIndex, i0, i1, i2))
        {
            continue;
        }

        glm::vec3 localV0 = mesh.getVertexPosition(i0);
        glm::vec3 localV1 = mesh.getVertexPosition(i1);
        glm::vec3 localV2 = mesh.getVertexPosition(i2);

        glm::vec3 worldV0 = glm::vec3(modelMatrix * glm::vec4(localV0, 1.0f));
        glm::vec3 worldV1 = glm::vec3(modelMatrix * glm::vec4(localV1, 1.0f));
        glm::vec3 worldV2 = glm::vec3(modelMatrix * glm::vec4(localV2, 1.0f));

        float t = 0.0f;

        if (intersectRayTriangle(ray, worldV0, worldV1, worldV2, t))
        {
            if (t < closestT)
            {
                closestT = t;
                selectedTriangleIndex = static_cast<int>(triangleIndex);
            }
        }
    }

    if (selectedTriangleIndex < 0)
    {
        return -1;
    }

    if (!mesh.hasLogicalFaces())
    {
        return selectedTriangleIndex;
    }

    return mesh.getLogicalFaceIndexFromTriangle(static_cast<unsigned int>(selectedTriangleIndex));
}