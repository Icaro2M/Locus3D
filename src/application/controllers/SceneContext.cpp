#include "SceneContext.h"
#include <string>

SceneContext::SceneContext()
{
}

void SceneContext::addPrimitive(int type)
{
    if (type < 0 || type > 3) return;

    static int cubeCount = 1, sphereCount = 1, coneCount = 1, cylinderCount = 1;
    std::string name;

    // 1. Cria a malha (Mesh) e guarda na memória de forma permanente
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
        // 2. Cria o Objeto apontando para a malha segura
        auto newObject = std::make_unique<SceneObject>(*newMesh);
        newObject->setName(name);

        // 3. Adiciona na Scene (A Scene agora recebe uma referência segura)
        m_Scene.addObject(*newObject);

        // 4. Guarda os ponteiros no nosso Contexto para eles nunca serem deletados
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