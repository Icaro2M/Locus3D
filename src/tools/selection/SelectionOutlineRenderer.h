#pragma once

#include "../../scene/Camera.h"
#include "../../scene/SceneObject.h"
#include "../../graphics/Shader.h"
#include "../../graphics/buffers/VertexArray.h"
#include "../../graphics/buffers/VertexBuffer.h"

class SelectionOutlineRenderer
{
private:
    Shader m_Shader;
    VertexArray m_VAO;
    VertexBuffer* m_VBO;

public:
    SelectionOutlineRenderer();
    ~SelectionOutlineRenderer();

    void render(const SceneObject& selectedObject, const Camera& camera);
};