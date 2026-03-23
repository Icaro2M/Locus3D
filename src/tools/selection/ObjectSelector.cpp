#include "ObjectSelector.h"

#include <glm/glm/glm.hpp>
#include <limits>

#include "Raycaster.h"

SceneObject* ObjectSelector::selectObject(GLFWwindow* window, const Camera& camera, const Scene& scene)
{
    Raycaster raycaster;
    Ray ray = raycaster.buildRayFromMouse(window, camera);

    glm::vec3 rayDirection = ray.direction;
    glm::vec3 rayOrigin = ray.origin;

    SceneObject* selectedObject = nullptr;
    float closestHit = std::numeric_limits<float>::max();

    for (SceneObject* object : scene.getObjects())
    {
        glm::vec3 sphereCenter = object->getTransform().getPosition();
        float sphereRadius = 1.0f;

        glm::vec3 oc = rayOrigin - sphereCenter;

        float a = glm::dot(rayDirection, rayDirection);
        float b = 2.0f * glm::dot(oc, rayDirection);
        float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;

        float discriminant = b * b - 4.0f * a * c;

        if (discriminant < 0.0f)
            continue;

        float sqrtDiscriminant = glm::sqrt(discriminant);

        float t1 = (-b - sqrtDiscriminant) / (2.0f * a);
        float t2 = (-b + sqrtDiscriminant) / (2.0f * a);

        float t = -1.0f;

        if (t1 > 0.0f)
            t = t1;
        else if (t2 > 0.0f)
            t = t2;

        if (t > 0.0f && t < closestHit)
        {
            closestHit = t;
            selectedObject = object;
        }
    }

    return selectedObject;
}