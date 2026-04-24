#include "SceneContext.h"

SceneContext::SceneContext()
{
}

void SceneContext::addPrimitive(int type)
{
    if (type < 0 || type > 3)
        return;

    if (type == 0)
    {
        addMeshAsObject(PrimitiveFactory::createCube());
    }
    else if (type == 1)
    {
        addMeshAsObject(PrimitiveFactory::createUvSphere(32, 16, 0.8f));
    }
    else if (type == 2)
    {
        addMeshAsObject(PrimitiveFactory::createCone(24, 1.5f, 4.0f));
    }
    else if (type == 3)
    {
        addMeshAsObject(PrimitiveFactory::createCylinder(24, 1.0f, 2.0f));
    }
}

void SceneContext::addObject(SceneObject& object)
{
    m_Scene.addObject(object);
}

Scene& SceneContext::getScene()
{
    return m_Scene;
}

const Scene& SceneContext::getScene() const
{
    return m_Scene;
}

void SceneContext::addMeshAsObject(Mesh&& mesh)
{
    m_Meshes.push_back(std::make_unique<Mesh>(std::move(mesh)));

    Mesh& storedMesh = *m_Meshes.back();

    m_Objects.push_back(std::make_unique<SceneObject>(storedMesh));

    SceneObject& storedObject = *m_Objects.back();

    m_Scene.addObject(storedObject);
}