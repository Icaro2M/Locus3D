#include "math/Transform.h"

#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/type_ptr.hpp>

Transform::Transform()
    : m_Position(0.0f, 0.0f, 0.0f),
    m_Rotation(0.0f, 0.0f, 0.0f),
    m_Scale(1.0f, 1.0f, 1.0f)
{
}

Transform::Transform(const glm::vec3& position,
    const glm::vec3& rotation,
    const glm::vec3& scale)
    : m_Position(position),
    m_Rotation(rotation),
    m_Scale(scale)
{
}

void Transform::setPosition(const glm::vec3& position)
{
    m_Position = position;
}

void Transform::setRotation(const glm::vec3& rotation)
{
    m_Rotation = rotation;
}

void Transform::setScale(const glm::vec3& scale)
{
    m_Scale = scale;
}

const glm::vec3& Transform::getPosition() const
{
    return m_Position;
}

const glm::vec3& Transform::getRotation() const
{
    return m_Rotation;
}

const glm::vec3& Transform::getScale() const
{
    return m_Scale;
}

glm::mat4 Transform::getModelMatrix() const
{
    glm::mat4 model = glm::mat4(1.0f);

    model = glm::translate(model, m_Position);

    model = glm::rotate(model, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

    model = glm::scale(model, m_Scale);

    return model;
}