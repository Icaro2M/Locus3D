#pragma once

#include "../scene/Camera.h"
#include "../graphics/Shader.h"
#include "../graphics/buffers/VertexArray.h"
#include "../graphics/buffers/VertexBuffer.h"

class GridRenderer
{
private:
    VertexArray m_VAO;
    VertexBuffer* m_VBO;
    Shader m_Shader;

    unsigned int m_VertexCount;

public:
    GridRenderer();
    ~GridRenderer();

    void render(const Camera& camera);
};