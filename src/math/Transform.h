#pragma once

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <glm/glm/gtc/quaternion.hpp>

class Transform
{
private:
	glm::vec3 m_Position;
	glm::vec3 m_Rotation;
	glm::quat m_Orientation;
	glm::vec3 m_Scale;

private:
	void updateOrientationFromEuler();
	void updateEulerFromOrientation();

public:
	Transform();

	const glm::vec3& getPosition() const;
	const glm::vec3& getRotation() const;
	const glm::vec3& getScale() const;
	const glm::quat& getOrientation() const;

	void setPosition(const glm::vec3& position);
	void setRotation(const glm::vec3& rotation);
	void setScale(const glm::vec3& scale);

	void rotateGlobal(float angleDegrees, const glm::vec3& axis);
	void rotateLocal(float angleDegrees, const glm::vec3& axis);

	glm::mat4 getModelMatrix() const;
};