#include "FaceHighlightRenderer.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>

FaceHighlightRenderer::FaceHighlightRenderer()
	:
	m_Shader(
		"C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\selection\\faceSelectionVertex.glsl",
		"C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\selection\\faceSelectionFragment.glsl"
	),
	m_VBO(nullptr)
{
	float vertices[] =
	{
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f
	};

	m_VAO.bind();

	m_VBO = new VertexBuffer(vertices, sizeof(vertices));
	m_VBO->bind();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	m_VAO.unbind();
	m_VBO->unbind();
}

FaceHighlightRenderer::~FaceHighlightRenderer()
{
	delete m_VBO;
}

void FaceHighlightRenderer::render(const SceneObject& object, int faceIndex, const Camera& camera)
{
	if (faceIndex < 0)
		return;

	const std::vector<float>& vertices = object.getMesh().getVertices();
	const std::vector<unsigned int>& indices = object.getMesh().getIndices();

	size_t triangleStart = static_cast<size_t>(faceIndex) * 3;

	if (triangleStart + 2 >= indices.size())
		return;

	unsigned int i0 = indices[triangleStart];
	unsigned int i1 = indices[triangleStart + 1];
	unsigned int i2 = indices[triangleStart + 2];

	size_t base0 = static_cast<size_t>(i0) * 6;
	size_t base1 = static_cast<size_t>(i1) * 6;
	size_t base2 = static_cast<size_t>(i2) * 6;

	if (base0 + 2 >= vertices.size() ||
		base1 + 2 >= vertices.size() ||
		base2 + 2 >= vertices.size())
	{
		return;
	}

	glm::vec3 localV0(vertices[base0], vertices[base0 + 1], vertices[base0 + 2]);
	glm::vec3 localV1(vertices[base1], vertices[base1 + 1], vertices[base1 + 2]);
	glm::vec3 localV2(vertices[base2], vertices[base2 + 1], vertices[base2 + 2]);

	glm::mat4 modelMatrix = object.getTransform().getModelMatrix();

	glm::vec3 worldV0 = glm::vec3(modelMatrix * glm::vec4(localV0, 1.0f));
	glm::vec3 worldV1 = glm::vec3(modelMatrix * glm::vec4(localV1, 1.0f));
	glm::vec3 worldV2 = glm::vec3(modelMatrix * glm::vec4(localV2, 1.0f));

	float highlightVertices[] =
	{
		worldV0.x, worldV0.y, worldV0.z,
		worldV1.x, worldV1.y, worldV1.z,
		worldV2.x, worldV2.y, worldV2.z
	};

	m_VBO->bind();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(highlightVertices), highlightVertices);

	m_Shader.use();
	m_Shader.setMat4("u_View", camera.getViewMatrix());
	m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());
	m_Shader.setMat4("u_Model", glm::mat4(1.0f));

	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1.0f, -1.0f);

	m_VAO.bind();
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisable(GL_POLYGON_OFFSET_FILL);
}