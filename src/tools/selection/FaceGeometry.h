#pragma once

#include "../../scene/SceneObject.h"
#include <glm/glm/glm.hpp>

class FaceGeometry
{
private:
	SceneObject* m_Object;
	int m_FaceIndex;

	glm::vec3 m_LocalV0;
	glm::vec3 m_LocalV1;
	glm::vec3 m_LocalV2;
	glm::vec3 m_LocalNormal;

public:
	FaceGeometry();

	void setObject(SceneObject* object);
	void setFaceIndex(int faceIndex);

	void setLocalVertices(
		const glm::vec3& v0,
		const glm::vec3& v1,
		const glm::vec3& v2
	);

	void setLocalNormal(const glm::vec3& normal);

	SceneObject* getObject() const;
	int getFaceIndex() const;

	const glm::vec3& getLocalV0() const;
	const glm::vec3& getLocalV1() const;
	const glm::vec3& getLocalV2() const;
	const glm::vec3& getLocalNormal() const;

	bool isValid() const;
	void clear();
};