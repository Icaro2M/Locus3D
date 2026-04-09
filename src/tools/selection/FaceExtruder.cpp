#include "FaceExtruder.h"

#include <vector>
#include <glm/glm/glm.hpp>

namespace
{
	void appendVertex(
		std::vector<float>& vertices,
		const glm::vec3& position,
		const glm::vec3& normal
	)
	{
		vertices.push_back(position.x);
		vertices.push_back(position.y);
		vertices.push_back(position.z);

		vertices.push_back(normal.x);
		vertices.push_back(normal.y);
		vertices.push_back(normal.z);
	}

	void appendTriangle(
		std::vector<unsigned int>& indices,
		unsigned int i0,
		unsigned int i1,
		unsigned int i2
	)
	{
		indices.push_back(i0);
		indices.push_back(i1);
		indices.push_back(i2);
	}

	glm::vec3 computeNormal(
		const glm::vec3& v0,
		const glm::vec3& v1,
		const glm::vec3& v2
	)
	{
		return glm::normalize(glm::cross(v1 - v0, v2 - v0));
	}

	void appendSideQuad(
		std::vector<float>& vertices,
		std::vector<unsigned int>& indices,
		const glm::vec3& baseV0,
		const glm::vec3& baseV1,
		const glm::vec3& topV1,
		const glm::vec3& topV0
	)
	{
		glm::vec3 sideNormal = computeNormal(baseV0, baseV1, topV1);

		unsigned int i0 = static_cast<unsigned int>(vertices.size() / 6);
		appendVertex(vertices, baseV0, sideNormal);

		unsigned int i1 = static_cast<unsigned int>(vertices.size() / 6);
		appendVertex(vertices, baseV1, sideNormal);

		unsigned int i2 = static_cast<unsigned int>(vertices.size() / 6);
		appendVertex(vertices, topV1, sideNormal);

		unsigned int i3 = static_cast<unsigned int>(vertices.size() / 6);
		appendVertex(vertices, topV0, sideNormal);

		appendTriangle(indices, i0, i1, i2);
		appendTriangle(indices, i0, i2, i3);
	}
}

bool FaceExtruder::extrude(FaceSelection& selection, float distance)
{
	if (!selection.isValid())
	{
		return false;
	}

	FaceGeometry geometry = m_FaceGeometryBuilder.build(selection);
	if (!geometry.isValid())
	{
		return false;
	}

	SceneObject* object = geometry.getObject();
	if (object == nullptr)
	{
		return false;
	}

	Mesh& mesh = object->getMesh();

	std::vector<float> newVertices = mesh.getVertices();
	std::vector<unsigned int> newIndices = mesh.getIndices();

	glm::vec3 v0 = geometry.getLocalV0();
	glm::vec3 v1 = geometry.getLocalV1();
	glm::vec3 v2 = geometry.getLocalV2();
	glm::vec3 normal = geometry.getLocalNormal();

	glm::vec3 extrudedV0 = v0 + normal * distance;
	glm::vec3 extrudedV1 = v1 + normal * distance;
	glm::vec3 extrudedV2 = v2 + normal * distance;

	unsigned int originalI0 = 0;
	unsigned int originalI1 = 0;
	unsigned int originalI2 = 0;

	bool validTriangle = mesh.getTriangleVertexIndices(
		static_cast<unsigned int>(geometry.getFaceIndex()),
		originalI0,
		originalI1,
		originalI2
	);

	if (!validTriangle)
	{
		return false;
	}

	std::vector<unsigned int> filteredIndices;
	unsigned int removedTriangleIndex = static_cast<unsigned int>(geometry.getFaceIndex());

	for (unsigned int i = 0; i + 2 < newIndices.size(); i += 3)
	{
		unsigned int currentTriangleIndex = i / 3;

		if (currentTriangleIndex == removedTriangleIndex)
		{
			continue;
		}

		filteredIndices.push_back(newIndices[i]);
		filteredIndices.push_back(newIndices[i + 1]);
		filteredIndices.push_back(newIndices[i + 2]);
	}

	newIndices = filteredIndices;

	unsigned int newTopFaceIndex = static_cast<unsigned int>(newIndices.size() / 3);

	glm::vec3 topNormal = normal;

	unsigned int topI0 = static_cast<unsigned int>(newVertices.size() / 6);
	appendVertex(newVertices, extrudedV0, topNormal);

	unsigned int topI1 = static_cast<unsigned int>(newVertices.size() / 6);
	appendVertex(newVertices, extrudedV1, topNormal);

	unsigned int topI2 = static_cast<unsigned int>(newVertices.size() / 6);
	appendVertex(newVertices, extrudedV2, topNormal);

	appendTriangle(newIndices, topI0, topI1, topI2);

	appendSideQuad(newVertices, newIndices, v0, v1, extrudedV1, extrudedV0);
	appendSideQuad(newVertices, newIndices, v1, v2, extrudedV2, extrudedV1);
	appendSideQuad(newVertices, newIndices, v2, v0, extrudedV0, extrudedV2);

	mesh.updateGeometry(newVertices, newIndices);

	selection.set(object, static_cast<int>(newTopFaceIndex));

	return true;
}