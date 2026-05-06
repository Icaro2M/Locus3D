#pragma once

#include "../../scene/Scene.h"
#include "../../scene/SceneObject.h"
#include "../../geometry/Mesh.h"
#include "../../geometry/primitives/PrimitiveFactory.h"
#include <memory>
#include <vector>

void removeObject(uint32_t id);

class SceneContext
{
public:
    SceneContext();
    ~SceneContext() = default;

    void addPrimitive(int type);

    void addCustomSolid(const char* name, int sides, float bottomRadius, float topRadius, float height);

    void addObject(SceneObject& object);
    void removeObject(uint32_t id);

    Scene& getScene();
    const Scene& getScene() const;

private:
    Scene m_Scene;

    // Estes vetores seguram a memória real dos objetos para eles não sumirem da tela
    std::vector<std::unique_ptr<Mesh>> m_Meshes;
    std::vector<std::unique_ptr<SceneObject>> m_Objects;
};