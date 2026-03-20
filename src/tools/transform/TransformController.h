#pragma once

#include "../../scene/SceneObject.h"

enum class TransformMode
{
	None,
	Translate,
	Rotate,
	Scale
};

enum class TransformAxis
{
	None,
	X,
	Y,
	Z
};

class TransformController
{
private:
	SceneObject* m_SelectedObject;
	TransformMode m_Mode;
	TransformAxis m_Axis;
	float m_TranslationStep;
	float m_RotationStep;
	float m_ScaleStep;

private:
	void applyStep(float direction);

public:
	TransformController();

	void setSelectedObject(SceneObject* selectedObject);

	TransformMode getMode() const;
	TransformAxis getAxis() const;

	void setMode(TransformMode mode);
	void setAxis(TransformAxis axis);

	void clearSelection();

	void applyPositiveStep();
	void applyNegativeStep();
};