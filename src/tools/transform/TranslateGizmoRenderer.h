#pragma once

#include "../../graphics/Shader.h"
#include "../../graphics/buffers/VertexArray.h"
#include "../../graphics/buffers/VertexBuffer.h"
#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"
#include "TransformController.h"

class TranslateGizmoRenderer
{
private:
    Shader m_Shader;
    VertexArray m_VAO;
    VertexBuffer* m_VBO;
    float m_GizmoSize;

public:
    TranslateGizmoRenderer();
    ~TranslateGizmoRenderer();

    void render(
        const SceneObject& selectedObject,
        const Camera& camera,
        TransformAxis activeAxis,
        TransformSpace transformSpace
    );
};