#pragma once

#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"

struct GLFWwindow;

class FaceSelector
{
public:
	int selectFace(const SceneObject& selectedObject, GLFWwindow* window, const Camera& camera) const;
};