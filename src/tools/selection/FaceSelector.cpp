#include "FaceSelector.h"

#include "Raycaster.h"

#include <limits>

#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

namespace
{
	bool intersectRayTriangle(
		const Ray& ray,
		const glm::vec3& v0,
		const glm::vec3& v1,
		const glm::vec3& v2,
		float& outT
	)
	{
		const float epsilon = 0.000001f;

		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;

		glm::vec3 h = glm::cross(ray.direction, edge2);
		float a = glm::dot(edge1, h);

		if (a > -epsilon && a < epsilon)
			return false;

		float f = 1.0f / a;
		glm::vec3 s = ray.origin - v0;
		float u = f * glm::dot(s, h);

		if (u < 0.0f || u > 1.0f)
			return false;

		glm::vec3 q = glm::cross(s, edge1);
		float v = f * glm::dot(ray.direction, q);

		if (v < 0.0f || u + v > 1.0f)
			return false;

		float t = f * glm::dot(edge2, q);

		if (t > epsilon)
		{
			outT = t;
			return true;
		}

		return false;
	}
}

int FaceSelector::selectFace(const SceneObject& selectedObject, GLFWwindow* window, const Camera& camera) const
{
	Raycaster raycaster;
	Ray ray = raycaster.buildRayFromMouse(window, camera);

	const std::vector<float>& vertices = selectedObject.getMesh().getVertices();
	const std::vector<unsigned int>& indices = selectedObject.getMesh().getIndices();

	if (indices.size() < 3 || vertices.size() < 6)
		return -1;

	glm::mat4 modelMatrix = selectedObject.getTransform().getModelMatrix();

	float closestT = std::numeric_limits<float>::max();
	int selectedFaceIndex = -1;

	for (size_t i = 0; i + 2 < indices.size(); i += 3)
	{
		unsigned int i0 = indices[i];
		unsigned int i1 = indices[i + 1];
		unsigned int i2 = indices[i + 2];

		size_t base0 = static_cast<size_t>(i0) * 6;
		size_t base1 = static_cast<size_t>(i1) * 6;
		size_t base2 = static_cast<size_t>(i2) * 6;

		if (base0 + 2 >= vertices.size() ||
			base1 + 2 >= vertices.size() ||
			base2 + 2 >= vertices.size())
		{
			continue;
		}

		glm::vec3 localV0(vertices[base0], vertices[base0 + 1], vertices[base0 + 2]);
		glm::vec3 localV1(vertices[base1], vertices[base1 + 1], vertices[base1 + 2]);
		glm::vec3 localV2(vertices[base2], vertices[base2 + 1], vertices[base2 + 2]);

		glm::vec3 worldV0 = glm::vec3(modelMatrix * glm::vec4(localV0, 1.0f));
		glm::vec3 worldV1 = glm::vec3(modelMatrix * glm::vec4(localV1, 1.0f));
		glm::vec3 worldV2 = glm::vec3(modelMatrix * glm::vec4(localV2, 1.0f));

		float t = 0.0f;
		if (intersectRayTriangle(ray, worldV0, worldV1, worldV2, t))
		{
			if (t < closestT)
			{
				closestT = t;
				selectedFaceIndex = static_cast<int>(i / 3);
			}
		}
	}

	return selectedFaceIndex;
}