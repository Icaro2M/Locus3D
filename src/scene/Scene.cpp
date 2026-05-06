#include "Scene.h"
#include <algorithm>
Scene::Scene() {}

void Scene::removeObject(uint32_t id)
{
    // Procura o objeto pelo ID e remove da lista de renderização
    // Nota: Substitua "m_Objects" pelo nome exato do seu vetor (ex: m_objects)
    m_Objects.erase(
        std::remove_if(m_Objects.begin(), m_Objects.end(),
            [id](SceneObject* obj) { return obj->getId() == id; }),
        m_Objects.end()
    );
}

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

