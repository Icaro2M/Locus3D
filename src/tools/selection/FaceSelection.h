#pragma once

#include "../../scene/SceneObject.h"

class FaceSelection
{
private:
	SceneObject* m_Object;
	int m_FaceIndex;

public:
	FaceSelection();

	void set(SceneObject* object, int faceIndex);
	void clear();

	SceneObject* getObject() const;
	int getFaceIndex() const;

	bool isValid() const;
};