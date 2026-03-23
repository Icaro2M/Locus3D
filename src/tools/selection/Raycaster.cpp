#include "Raycaster.h"

#include <GLFW/glfw3.h>
#include <glm/glm/gtc/matrix_inverse.hpp>


Ray Raycaster::buildRayFromMouse(GLFWwindow* window, const Camera& camera) const
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

    return Ray{ rayOrigin, rayWorld };
}
