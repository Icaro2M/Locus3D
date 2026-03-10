#pragma once

#include "geometry/Mesh.h"
#include "graphics/Shader.h"

class Renderer
{
public:

    void clear() const;

    void draw(const Mesh& mesh, const Shader& shader) const;
};