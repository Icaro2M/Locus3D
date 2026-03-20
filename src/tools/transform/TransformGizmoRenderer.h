#pragma once

#include "../../graphics/Shader.h"
#include "../../graphics/buffers/VertexArray.h"
#include "../../graphics/buffers/VertexBuffer.h"
#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"

class TransformGizmoRenderer
{
private:
	Shader m_Shader;
	VertexArray m_VAO;
	VertexBuffer* m_VBO;
	float m_GizmoSize;

public:
	TransformGizmoRenderer();
	~TransformGizmoRenderer();

	void render(const SceneObject& selectedObject, const Camera& camera);
};