#include "SceneObject.h"
#include <string>

// Inicialização da variável estática
uint32_t SceneObject::s_nextId = 1;



SceneObject::SceneObject(Mesh& mesh) 
    : m_Mesh(mesh) 
{
    // Atribui um ID único e incrementa o contador global
    m_id = s_nextId++;
    
    // Define um nome padrão baseado no ID (ex: "Objeto 1")
    m_name = "Objeto " + std::to_string(m_id);
}

Mesh& SceneObject::getMesh()
{
    return m_Mesh;
}

const Mesh& SceneObject::getMesh() const
{
    return m_Mesh;
}

Transform& SceneObject::getTransform()
{
    return m_Transform;
}

const Transform& SceneObject::getTransform() const
{
    return m_Transform;
}

uint32_t SceneObject::getId() const
{
    return m_id;
}

std::string SceneObject::getName() const
{
    return m_name;
}

void SceneObject::setName(const std::string& name)
{
    m_name = name;
}
