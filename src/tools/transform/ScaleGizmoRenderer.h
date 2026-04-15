#pragma once

#include "../../graphics/Shader.h"
#include "../../graphics/buffers/VertexArray.h"
#include "../../graphics/buffers/VertexBuffer.h"
#include "../../graphics/buffers/IndexBuffer.h"
#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"
#include "TransformController.h"

class ScaleGizmoRenderer
{
private:
    Shader m_LineShader;
    Shader m_HandleShader;

    VertexArray m_LineVAO;
    VertexBuffer* m_LineVBO;

    VertexArray m_HandleVAO;
    VertexBuffer* m_HandleVBO;
    IndexBuffer* m_HandleEBO;

    float m_GizmoSize;
    float m_HandleSize;

public:
    ScaleGizmoRenderer();
    ~ScaleGizmoRenderer();

    void render(
        const SceneObject& selectedObject,
        const Camera& camera,
        TransformAxis activeAxis,
        TransformSpace transformSpace
    );
};