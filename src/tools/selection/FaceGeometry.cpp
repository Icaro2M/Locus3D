#include "FaceGeometry.h"

FaceGeometry::FaceGeometry()
	:
	m_Object(nullptr),
	m_FaceIndex(-1),
	m_LocalV0(0.0f, 0.0f, 0.0f),
	m_LocalV1(0.0f, 0.0f, 0.0f),
	m_LocalV2(0.0f, 0.0f, 0.0f),
	m_LocalNormal(0.0f, 0.0f, 0.0f)
{
}

void FaceGeometry::setObject(SceneObject* object)
{
	m_Object = object;
}

void FaceGeometry::setFaceIndex(int faceIndex)
{
	m_FaceIndex = faceIndex;
}

void FaceGeometry::setLocalVertices(
	const glm::vec3& v0,
	const glm::vec3& v1,
	const glm::vec3& v2
)
{
	m_LocalV0 = v0;
	m_LocalV1 = v1;
	m_LocalV2 = v2;
}

void FaceGeometry::setLocalNormal(const glm::vec3& normal)
{
	m_LocalNormal = normal;
}

SceneObject* FaceGeometry::getObject() const
{
	return m_Object;
}

int FaceGeometry::getFaceIndex() const
{
	return m_FaceIndex;
}

const glm::vec3& FaceGeometry::getLocalV0() const
{
	return m_LocalV0;
}

const glm::vec3& FaceGeometry::getLocalV1() const
{
	return m_LocalV1;
}

const glm::vec3& FaceGeometry::getLocalV2() const
{
	return m_LocalV2;
}

const glm::vec3& FaceGeometry::getLocalNormal() const
{
	return m_LocalNormal;
}

bool FaceGeometry::isValid() const
{
	return m_Object != nullptr && m_FaceIndex >= 0;
}

void FaceGeometry::clear()
{
	m_Object = nullptr;
	m_FaceIndex = -1;

	m_LocalV0 = glm::vec3(0.0f, 0.0f, 0.0f);
	m_LocalV1 = glm::vec3(0.0f, 0.0f, 0.0f);
	m_LocalV2 = glm::vec3(0.0f, 0.0f, 0.0f);
	m_LocalNormal = glm::vec3(0.0f, 0.0f, 0.0f);
}