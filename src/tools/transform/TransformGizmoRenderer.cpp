#include "TransformGizmoRenderer.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

TransformGizmoRenderer::TransformGizmoRenderer()
	:
	m_Shader(
		"C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\transformGizmo\\vertex.glsl",
		"C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\helpers\\transformGizmo\\fragment.glsl"
	),
	m_VBO(nullptr),
	m_GizmoSize(1.5f)
{
	float vertices[] =
	{
		0.0f,        0.0f,        0.0f,        1.0f, 0.0f, 0.0f,
		m_GizmoSize, 0.0f,        0.0f,        1.0f, 0.0f, 0.0f,

		0.0f,        0.0f,        0.0f,        0.0f, 1.0f, 0.0f,
		0.0f,        m_GizmoSize, 0.0f,        0.0f, 1.0f, 0.0f,

		0.0f,        0.0f,        0.0f,        0.0f, 0.0f, 1.0f,
		0.0f,        0.0f,        m_GizmoSize, 0.0f, 0.0f, 1.0f
	};

	m_VAO.bind();

	m_VBO = new VertexBuffer(vertices, sizeof(vertices));
	m_VBO->bind();

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	m_VAO.unbind();
	m_VBO->unbind();
}

TransformGizmoRenderer::~TransformGizmoRenderer()
{
	delete m_VBO;
}

void TransformGizmoRenderer::render(
	const SceneObject& selectedObject,
	const Camera& camera,
	TransformAxis activeAxis,
	TransformSpace transformSpace
)
{
	glm::vec3 xColor(1.0f, 0.0f, 0.0f);
	glm::vec3 yColor(0.0f, 1.0f, 0.0f);
	glm::vec3 zColor(0.0f, 0.0f, 1.0f);

	glm::vec3 dimXColor(0.35f, 0.0f, 0.0f);
	glm::vec3 dimYColor(0.0f, 0.35f, 0.0f);
	glm::vec3 dimZColor(0.0f, 0.0f, 0.35f);

	glm::vec3 finalXColor = xColor;
	glm::vec3 finalYColor = yColor;
	glm::vec3 finalZColor = zColor;

	switch (activeAxis)
	{
	case TransformAxis::X:
		finalYColor = dimYColor;
		finalZColor = dimZColor;
		break;

	case TransformAxis::Y:
		finalXColor = dimXColor;
		finalZColor = dimZColor;
		break;

	case TransformAxis::Z:
		finalXColor = dimXColor;
		finalYColor = dimYColor;
		break;

	case TransformAxis::None:
		break;
	}

	float vertices[] =
	{
		0.0f,        0.0f,        0.0f,        finalXColor.r, finalXColor.g, finalXColor.b,
		m_GizmoSize, 0.0f,        0.0f,        finalXColor.r, finalXColor.g, finalXColor.b,

		0.0f,        0.0f,        0.0f,        finalYColor.r, finalYColor.g, finalYColor.b,
		0.0f,        m_GizmoSize, 0.0f,        finalYColor.r, finalYColor.g, finalYColor.b,

		0.0f,        0.0f,        0.0f,        finalZColor.r, finalZColor.g, finalZColor.b,
		0.0f,        0.0f,        m_GizmoSize, finalZColor.r, finalZColor.g, finalZColor.b
	};

	m_VBO->bind();
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	m_Shader.use();
	m_Shader.setMat4("u_View", camera.getViewMatrix());
	m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

	glm::mat4 model(1.0f);

	const glm::vec3& position = selectedObject.getTransform().getPosition();
	const glm::vec3& rotation = selectedObject.getTransform().getRotation();

	model = glm::translate(model, position);

	if (transformSpace == TransformSpace::Local)
	{
		model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	}

	m_Shader.setMat4("u_Model", model);

	m_VAO.bind();
	glDrawArrays(GL_LINES, 0, 6);
}