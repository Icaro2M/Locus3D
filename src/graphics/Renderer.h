#pragma once

#include "geometry/Mesh.h"
#include "graphics/Shader.h"
#include "scene/Scene.h"
#include "scene/Camera.h"

class Renderer
{
public:
    void clear() const;

    void draw(const Mesh& mesh, const Shader& shader) const;

    void renderScene(const Scene& scene, const Camera& camera, Shader& shader) const;
};