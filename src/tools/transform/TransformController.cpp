#include "TransformController.h"
#include <glm/glm/glm.hpp>

TransformController::TransformController()
	:
	m_SelectedObject(nullptr),
	m_Mode(TransformMode::None),
	m_Axis(TransformAxis::None),
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

void TransformController::setMode(TransformMode mode)
{
	m_Mode = mode;
}

void TransformController::setAxis(TransformAxis axis)
{
	m_Axis = axis;
}

void TransformController::clearSelection()
{
	m_SelectedObject = nullptr;
	m_Mode = TransformMode::None;
	m_Axis = TransformAxis::None;
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
		glm::vec3 currentPosition = m_SelectedObject->getTransform().getPosition();
		glm::vec3 newPosition = currentPosition + (m_TranslationStep * direction * axisVector);
		m_SelectedObject->getTransform().setPosition(newPosition);
		break;
	}

	case TransformMode::Rotate:
	{
		glm::vec3 currentRotation = m_SelectedObject->getTransform().getRotation();
		glm::vec3 newRotation = currentRotation + (m_RotationStep * direction * axisVector);
		m_SelectedObject->getTransform().setRotation(newRotation);
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