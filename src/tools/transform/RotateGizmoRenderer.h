#pragma once

#include "../../graphics/Shader.h"
#include "../../graphics/buffers/VertexArray.h"
#include "../../graphics/buffers/VertexBuffer.h"
#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"
#include "TransformTypes.h"

class RotateGizmoRenderer
{
private:
    Shader m_Shader;
    VertexArray m_VAO;
    VertexBuffer* m_VBO;
    float m_Radius;
    int m_Segments;

public:
    RotateGizmoRenderer();
    ~RotateGizmoRenderer();

    void render(
        const SceneObject& selectedObject,
        const Camera& camera,
        TransformAxis activeAxis,
        TransformSpace transformSpace
    );
};