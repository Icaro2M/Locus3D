#include "Scene.h"

Scene::Scene() {}

void Scene::addObject(SceneObject& object)
{
	m_Objects.push_back(&object);
}

std::vector<SceneObject*>& Scene::getObjects()
{
	return m_Objects;
}

const std::vector<SceneObject*>& Scene::getObjects() const
{
	return m_Objects;
}

