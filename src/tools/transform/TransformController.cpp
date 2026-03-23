#include "TransformController.h"
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

TransformController::TransformController()
	:
	m_SelectedObject(nullptr),
	m_Mode(TransformMode::None),
	m_Axis(TransformAxis::None),
	m_Space(TransformSpace::Global),
	m_TranslationStep(0.3f),
	m_RotationStep(5.0f),
	m_ScaleStep(0.1f)
{
}

void TransformController::setSelectedObject(SceneObject* selectedObject)
{
	m_SelectedObject = selectedObject;
}

TransformMode TransformController::getMode() const
{
	return m_Mode;
}

TransformAxis TransformController::getAxis() const
{
	return m_Axis;
}

TransformSpace TransformController::getSpace() const
{
	return m_Space;
}

void TransformController::setMode(TransformMode mode)
{
	m_Mode = mode;
}

void TransformController::setAxis(TransformAxis axis)
{
	m_Axis = axis;
}

void TransformController::setSpace(TransformSpace space)
{
	m_Space = space;
}

void TransformController::clearSelection()
{
	m_SelectedObject = nullptr;
	m_Mode = TransformMode::None;
	m_Axis = TransformAxis::None;
	m_Space = TransformSpace::Global;
}

void TransformController::applyPositiveStep()
{
	applyStep(1.0f);
}

void TransformController::applyNegativeStep()
{
	applyStep(-1.0f);
}

void TransformController::applyStep(float direction)
{
	if (m_SelectedObject == nullptr)
		return;

	if (m_Mode == TransformMode::None || m_Axis == TransformAxis::None)
		return;

	glm::vec3 axisVector(0.0f, 0.0f, 0.0f);

	switch (m_Axis)
	{
	case TransformAxis::X:
		axisVector.x = 1.0f;
		break;

	case TransformAxis::Y:
		axisVector.y = 1.0f;
		break;

	case TransformAxis::Z:
		axisVector.z = 1.0f;
		break;

	case TransformAxis::None:
		return;
	}

	switch (m_Mode)
	{
	case TransformMode::Translate:
	{
		glm::vec3 movementDirection = axisVector;

		if (m_Space == TransformSpace::Local)
		{
			const glm::vec3& rotation = m_SelectedObject->getTransform().getRotation();

			glm::mat4 rotationMatrix(1.0f);
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
			rotationMatrix = glm::rotate(rotationMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

			movementDirection = glm::vec3(rotationMatrix * glm::vec4(axisVector, 0.0f));
		}

		glm::vec3 currentPosition = m_SelectedObject->getTransform().getPosition();
		glm::vec3 newPosition = currentPosition + (m_TranslationStep * direction * movementDirection);
		m_SelectedObject->getTransform().setPosition(newPosition);
		break;
	}

	case TransformMode::Rotate:
	{
		float angle = m_RotationStep * direction;

		switch (m_Space)
		{
		case TransformSpace::Global:
			m_SelectedObject->getTransform().rotateGlobal(angle, axisVector);
			break;

		case TransformSpace::Local:
			m_SelectedObject->getTransform().rotateLocal(angle, axisVector);
			break;
		}

		break;
	}

	case TransformMode::Scale:
	{
		glm::vec3 currentScale = m_SelectedObject->getTransform().getScale();
		glm::vec3 newScale = currentScale + (m_ScaleStep * direction * axisVector);

		if (newScale.x < 0.1f) newScale.x = 0.1f;
		if (newScale.y < 0.1f) newScale.y = 0.1f;
		if (newScale.z < 0.1f) newScale.z = 0.1f;

		m_SelectedObject->getTransform().setScale(newScale);
		break;
	}

	case TransformMode::None:
		return;
	}
}