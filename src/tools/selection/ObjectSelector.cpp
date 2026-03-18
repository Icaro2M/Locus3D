#include "ObjectSelector.h"

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>
#include <limits>

SceneObject* ObjectSelector::selectObject(GLFWwindow* window, const Camera& camera, const Scene& scene)
{
    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    float x = (2.0f * static_cast<float>(mouseX)) / static_cast<float>(windowWidth) - 1.0f;
    float y = 1.0f - (2.0f * static_cast<float>(mouseY)) / static_cast<float>(windowHeight);

    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    glm::mat4 projection = camera.getProjectionMatrix();
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    glm::mat4 view = camera.getViewMatrix();
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));

    glm::vec3 rayOrigin = camera.getPosition();

    SceneObject* selectedObject = nullptr;
    float closestHit = std::numeric_limits<float>::max();

    for (SceneObject* object : scene.getObjects())
    {
        glm::vec3 sphereCenter = object->getTransform().getPosition();
        float sphereRadius = 1.0f;

        glm::vec3 oc = rayOrigin - sphereCenter;

        float a = glm::dot(rayWorld, rayWorld);
        float b = 2.0f * glm::dot(oc, rayWorld);
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