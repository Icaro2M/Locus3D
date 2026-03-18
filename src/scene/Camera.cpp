#include "scene/Camera.h"

#include <glm/glm/gtc/matrix_transform.hpp>

Camera::Camera()
    : m_Position(0.0f, 0.0f, 3.0f),
    m_Target(0.0f, 0.0f, 0.0f),
    m_Up(0.0f, 1.0f, 0.0f),
    m_Fov(45.0f),
    m_AspectRatio(800.0f / 600.0f),
    m_NearPlane(0.1f),
    m_FarPlane(100.0f),
    m_Yaw(90.0f),
    m_Pitch(0.0f),
    m_Distance(3.0f)
{
    updateOrbitPosition();
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

void Camera::setYaw(float yaw)
{
    m_Yaw = yaw;
}

void Camera::setPitch(float pitch)
{
    m_Pitch = pitch;
}

void Camera::setDistance(float distance)
{
    m_Distance = distance;
}

void Camera::updateOrbitPosition()
{
    float yawRad = glm::radians(m_Yaw);
    float pitchRad = glm::radians(m_Pitch);

    m_Position.x = m_Target.x + m_Distance * glm::cos(pitchRad) * glm::cos(yawRad);
    m_Position.y = m_Target.y + m_Distance * glm::sin(pitchRad);
    m_Position.z = m_Target.z + m_Distance * glm::cos(pitchRad) * glm::sin(yawRad);
}

void Camera::translateTarget(const glm::vec3& offset)
{
    m_Target += offset;
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

float Camera::getYaw() const
{
    return m_Yaw;
}

float Camera::getPitch() const
{
    return m_Pitch;
}

float Camera::getDistance() const
{
    return m_Distance;
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(m_Position, m_Target, m_Up);
}

glm::mat4 Camera::getProjectionMatrix() const
{
    return glm::perspective(glm::radians(m_Fov), m_AspectRatio, m_NearPlane, m_FarPlane);
}
