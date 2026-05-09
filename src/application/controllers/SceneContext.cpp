#include "SceneContext.h"
#include <string>
#include <algorithm>

SceneContext::SceneContext()
{
}

void SceneContext::removeObject(uint32_t id)
{
    m_Scene.removeObject(id);

    m_Objects.erase(
        std::remove_if(m_Objects.begin(), m_Objects.end(),
            [id](const std::unique_ptr<SceneObject>& obj) { return obj->getId() == id; }),
        m_Objects.end()
    );
}

void SceneContext::addPrimitive(int type)
{
    if (type < 0 || type > 3) return;

    static int cubeCount = 1, sphereCount = 1, coneCount = 1, cylinderCount = 1;
    std::string name;

    std::unique_ptr<Mesh> newMesh;

    if (type == 0) 
    {
        newMesh = std::make_unique<Mesh>(PrimitiveFactory::createCube());
        name = cubeCount == 1 ? "Cubo" : "Cubo " + std::to_string(cubeCount);
        cubeCount++;
    }
    else if (type == 1) 
    {
        newMesh = std::make_unique<Mesh>(PrimitiveFactory::createUvSphere(32, 16, 0.8f));
        name = sphereCount == 1 ? "Esfera" : "Esfera " + std::to_string(sphereCount);
        sphereCount++;
    }
    else if (type == 2) 
    {
        newMesh = std::make_unique<Mesh>(PrimitiveFactory::createCone(24, 1.5f, 4.0f));
        name = coneCount == 1 ? "Cone" : "Cone " + std::to_string(coneCount);
        coneCount++;
    }
    else if (type == 3)
    {
        newMesh = std::make_unique<Mesh>(PrimitiveFactory::createCylinder(24, 1.0f, 2.0f));
        name = cylinderCount == 1 ? "Cilindro" : "Cilindro " + std::to_string(cylinderCount);
        cylinderCount++;
    }

    if (newMesh)
    {
        auto newObject = std::make_unique<SceneObject>(*newMesh);
        newObject->setName(name);

        m_Scene.addObject(*newObject);

        m_Meshes.push_back(std::move(newMesh));
        m_Objects.push_back(std::move(newObject));
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

void SceneContext::addCustomSolid(const char* name, int sides, float bottomRadius, float topRadius, float height)
{
    bool capBottom = bottomRadius > 0.001f;
    bool capTop = topRadius > 0.001f;

    auto newMesh = std::make_unique<Mesh>(PrimitiveFactory::createRadialSolid(sides, height, bottomRadius, topRadius, capBottom, capTop));

    if (newMesh)
    {
        auto newObject = std::make_unique<SceneObject>(*newMesh);

        std::string objName = (name && name[0] != '\0') ? name : "Solido Customizado";
        newObject->setName(objName);

        m_Scene.addObject(*newObject);

        m_Meshes.push_back(std::move(newMesh));
        m_Objects.push_back(std::move(newObject));
    }
}