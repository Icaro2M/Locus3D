#pragma once

#include "SceneObject.h"
#include <vector>

class Scene
{
private:

    std::vector<SceneObject*> m_Objects;

public:
    Scene();

    void addObject(SceneObject& object);

    std::vector<SceneObject*>& getObjects();
    const std::vector<SceneObject*>& getObjects() const;

};