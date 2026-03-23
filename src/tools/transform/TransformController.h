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

enum class TransformSpace
{
	Global,
	Local
};

class TransformController
{
private:
	SceneObject* m_SelectedObject;
	TransformMode m_Mode;
	TransformAxis m_Axis;
	TransformSpace m_Space;
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
	TransformSpace getSpace() const;

	void setMode(TransformMode mode);
	void setAxis(TransformAxis axis);
	void setSpace(TransformSpace space);

	void clearSelection();

	void applyPositiveStep();
	void applyNegativeStep();
};