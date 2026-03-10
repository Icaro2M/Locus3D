#pragma once

#include "graphics/VertexArray.h"
#include "graphics/VertexBuffer.h"

class Mesh
{
private:

    VertexArray vao;
    VertexBuffer* vbo;

    unsigned int vertexCount;

public:

    Mesh(const float* vertices, unsigned int size);

    ~Mesh();

    const VertexArray& getVAO() const;

    unsigned int getVertexCount() const;
};