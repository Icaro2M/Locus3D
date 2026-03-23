#include "Transform.h"

Transform::Transform()
	:
	m_Position(0.0f, 0.0f, 0.0f),
	m_Rotation(0.0f, 0.0f, 0.0f),
	m_Orientation(glm::quat(glm::vec3(0.0f, 0.0f, 0.0f))),
	m_Scale(1.0f, 1.0f, 1.0f)
{
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

const glm::quat& Transform::getOrientation() const
{
	return m_Orientation;
}

void Transform::setPosition(const glm::vec3& position)
{
	m_Position = position;
}

void Transform::setRotation(const glm::vec3& rotation)
{
	m_Rotation = rotation;
	updateOrientationFromEuler();
}

void Transform::setScale(const glm::vec3& scale)
{
	m_Scale = scale;
}

void Transform::rotateGlobal(float angleDegrees, const glm::vec3& axis)
{
	glm::quat deltaRotation = glm::angleAxis(glm::radians(angleDegrees), glm::normalize(axis));
	m_Orientation = glm::normalize(deltaRotation * m_Orientation);
	updateEulerFromOrientation();
}

void Transform::rotateLocal(float angleDegrees, const glm::vec3& axis)
{
	glm::quat deltaRotation = glm::angleAxis(glm::radians(angleDegrees), glm::normalize(axis));
	m_Orientation = glm::normalize(m_Orientation * deltaRotation);
	updateEulerFromOrientation();
}

glm::mat4 Transform::getModelMatrix() const
{
	glm::mat4 model(1.0f);

	model = glm::translate(model, m_Position);
	model *= glm::mat4_cast(m_Orientation);
	model = glm::scale(model, m_Scale);

	return model;
}

void Transform::updateOrientationFromEuler()
{
	glm::vec3 radians = glm::radians(m_Rotation);
	m_Orientation = glm::quat(radians);
	m_Orientation = glm::normalize(m_Orientation);
}

void Transform::updateEulerFromOrientation()
{
	glm::vec3 radians = glm::eulerAngles(m_Orientation);
	m_Rotation = glm::degrees(radians);
}