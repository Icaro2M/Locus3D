#pragma once

#include "../../scene/SceneObject.h"
#include "../../scene/Scene.h"
#include "../../scene/Camera.h"

struct GLFWwindow;

class ObjectSelector
{
public:
    SceneObject* selectObject(GLFWwindow* window, const Camera& camera, const Scene& scene);
};