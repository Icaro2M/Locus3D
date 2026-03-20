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
		// X - vermelho
		0.0f,        0.0f,        0.0f,        1.0f, 0.0f, 0.0f,
		m_GizmoSize, 0.0f,        0.0f,        1.0f, 0.0f, 0.0f,

		// Y - verde
		0.0f,        0.0f,        0.0f,        0.0f, 1.0f, 0.0f,
		0.0f,        m_GizmoSize, 0.0f,        0.0f, 1.0f, 0.0f,

		// Z - azul
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

void TransformGizmoRenderer::render(const SceneObject& selectedObject, const Camera& camera)
{
	m_Shader.use();

	m_Shader.setMat4("u_View", camera.getViewMatrix());
	m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

	glm::mat4 model(1.0f);
	model = glm::translate(model, selectedObject.getTransform().getPosition());

	m_Shader.setMat4("u_Model", model);

	m_VAO.bind();
	glDrawArrays(GL_LINES, 0, 6);
}