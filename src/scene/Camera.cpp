#include "scene/Camera.h"

#include <glm/glm/gtc/matrix_transform.hpp>

Camera::Camera()
    : m_Position(0.0f, 0.0f, 3.0f),
    m_Target(0.0f, 0.0f, 0.0f),
    m_Up(0.0f, 1.0f, 0.0f),
    m_Fov(45.0f),
    m_AspectRatio(800.0f / 600.0f),
    m_NearPlane(0.1f),
    m_FarPlane(100.0f)
{
}

Camera::Camera(const glm::vec3& position,
    const glm::vec3& target,
    const glm::vec3& up,
    float fov,
    float aspectRatio,
    float nearPlane,
    float farPlane)
    : m_Position(position),
    m_Target(target),
    m_Up(up),
    m_Fov(fov),
    m_AspectRatio(aspectRatio),
    m_NearPlane(nearPlane),
    m_FarPlane(farPlane)
{
}

void Camera::setPosition(const glm::vec3& position)
{
    m_Position = position;
}

void Camera::setTarget(const glm::vec3& target)
{
    m_Target = target;
}

void Camera::setUp(const glm::vec3& up)
{
    m_Up = up;
}

void Camera::setFov(float fov)
{
    m_Fov = fov;
}

void Camera::setAspectRatio(float aspectRatio)
{
    m_AspectRatio = aspectRatio;
}

void Camera::setNearPlane(float nearPlane)
{
    m_NearPlane = nearPlane;
}

void Camera::setFarPlane(float farPlane)
{
    m_FarPlane = farPlane;
}

const glm::vec3& Camera::getPosition() const
{
    return m_Position;
}

const glm::vec3& Camera::getTarget() const
{
    return m_Target;
}

const glm::vec3& Camera::getUp() const
{
    return m_Up;
}

float Camera::getFov() const
{
    return m_Fov;
}

float Camera::getAspectRatio() const
{
    return m_AspectRatio;
}

float Camera::getNearPlane() const
{
    return m_NearPlane;
}

float Camera::getFarPlane() const
{
    return m_FarPlane;
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(m_Position, m_Target, m_Up);
}

glm::mat4 Camera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearPlane, m_FarPlane);
}
