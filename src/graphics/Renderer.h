#pragma once

#include "VertexArray.h"
#include "Shader.h"

class Renderer
{
public:

    void clear() const;

    void draw(const VertexArray& vao, const Shader& shader) const;

};