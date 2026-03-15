
#include "SceneObject.h"


SceneObject::SceneObject(Mesh& mesh) : m_Mesh(mesh) {}


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