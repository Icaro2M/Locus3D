#pragma once

#include <glm/glm/glm.hpp>

class Camera
{
private:
    glm::vec3 m_Position;
    glm::vec3 m_Target;
    glm::vec3 m_Up;

    float m_Fov;
    float m_AspectRatio;
    float m_NearPlane;
    float m_FarPlane;

public:
    Camera();

    Camera(const glm::vec3& position,
        const glm::vec3& target,
        const glm::vec3& up,
        float fov,
        float aspectRatio,
        float nearPlane,
        float farPlane);

    void setPosition(const glm::vec3& position);
    void setTarget(const glm::vec3& target);
    void setUp(const glm::vec3& up);

    void setFov(float fov);
    void setAspectRatio(float aspectRatio);
    void setNearPlane(float nearPlane);
    void setFarPlane(float farPlane);

    const glm::vec3& getPosition() const;
    const glm::vec3& getTarget() const;
    const glm::vec3& getUp() const;

    float getFov() const;
    float getAspectRatio() const;
    float getNearPlane() const;
    float getFarPlane() const;

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix() const;
};
