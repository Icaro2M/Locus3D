#pragma once

#include "../../scene/SceneObject.h"
#include "../../scene/Scene.h"
#include "../../scene/Camera.h"
#include <GLFW/glfw3.h>

class ObjectSelector
{
public:

	SceneObject* selectObject(GLFWwindow* window, const Camera& camera, const Scene& scene);

};