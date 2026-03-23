#pragma once

#include "../../scene/Camera.h"
#include "../../math/Ray.h"


struct GLFWwindow;

class Raycaster
{
public:
	Ray buildRayFromMouse(GLFWwindow* window, const Camera& camera) const;
};