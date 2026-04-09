#include "FaceGeometryBuilder.h"

#include <glm/glm/glm.hpp>

FaceGeometry FaceGeometryBuilder::build(const FaceSelection& selection) const
{
	FaceGeometry geometry;

	if (!selection.isValid())
	{
		return geometry;
	}

	SceneObject* object = selection.getObject();
	if (object == nullptr)
	{
		return geometry;
	}

	const Mesh& mesh = object->getMesh();

	unsigned int i0 = 0;
	unsigned int i1 = 0;
	unsigned int i2 = 0;

	bool validTriangle = mesh.getTriangleVertexIndices(
		static_cast<unsigned int>(selection.getFaceIndex()),
		i0,
		i1,
		i2
	);

	if (!validTriangle)
	{
		return geometry;
	}

	glm::vec3 localV0 = mesh.getVertexPosition(i0);
	glm::vec3 localV1 = mesh.getVertexPosition(i1);
	glm::vec3 localV2 = mesh.getVertexPosition(i2);

	glm::vec3 edge1 = localV1 - localV0;
	glm::vec3 edge2 = localV2 - localV0;

	glm::vec3 localNormal = glm::normalize(glm::cross(edge1, edge2));

	geometry.setObject(object);
	geometry.setFaceIndex(selection.getFaceIndex());
	geometry.setLocalVertices(localV0, localV1, localV2);
	geometry.setLocalNormal(localNormal);

	return geometry;
}