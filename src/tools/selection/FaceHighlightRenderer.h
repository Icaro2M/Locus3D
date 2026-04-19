#pragma once

#include "../../graphics/Shader.h"
#include "../../graphics/buffers/VertexArray.h"
#include "../../graphics/buffers/VertexBuffer.h"
#include "../../scene/Camera.h"
#include "../../scene/SceneObject.h"

class FaceHighlightRenderer
{
private:
    Shader m_Shader;
    VertexArray m_VAO;
    VertexBuffer* m_VBO;

public:
    FaceHighlightRenderer();
    ~FaceHighlightRenderer();

    void render(const SceneObject& object, int faceIndex, const Camera& camera);
};