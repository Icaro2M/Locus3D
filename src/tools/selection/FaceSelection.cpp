#include "FaceSelection.h"

FaceSelection::FaceSelection()
	:
	m_Object(nullptr),
	m_FaceIndex(-1)
{
}

void FaceSelection::set(SceneObject* object, int faceIndex)
{
	m_Object = object;
	m_FaceIndex = faceIndex;
}

SceneObject* FaceSelection::getObject() const
{
	return m_Object;
}

int FaceSelection::getFaceIndex() const
{
	return m_FaceIndex;
}

bool FaceSelection::isValid() const
{
	return m_Object != nullptr && m_FaceIndex >= 0;
}

void FaceSelection::clear()
{
	m_Object = nullptr;
	m_FaceIndex = -1;
}