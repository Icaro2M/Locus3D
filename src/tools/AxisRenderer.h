#pragma once

#include "../scene/Camera.h"
#include "../graphics/Shader.h"
#include "../graphics/buffers/VertexArray.h"
#include "../graphics/buffers/VertexBuffer.h"

class AxisRenderer
{
private:
    VertexArray m_VAO;
    VertexBuffer* m_VBO;
    Shader m_Shader;

    static constexpr unsigned int m_VertexCount = 6;

public:
    AxisRenderer();
    ~AxisRenderer();

    void render(const Camera& camera);
};