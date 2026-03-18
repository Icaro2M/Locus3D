#pragma once

#include "../../scene/Camera.h"
#include "../../scene/SceneObject.h"
#include "../../graphics/Shader.h"

class SelectionOutlineRenderer
{
private:
	Shader m_Shader;

public:

	SelectionOutlineRenderer();

	void render(const SceneObject& selectedObject, const Camera& camera);

};