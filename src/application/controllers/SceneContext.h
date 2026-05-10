#pragma once

#include "../../scene/Scene.h"
#include "../../scene/SceneObject.h"
#include "../../geometry/Mesh.h"
#include "../../geometry/primitives/PrimitiveFactory.h"

#include "../../io/SceneSaveData.h"
#include <string>

#include <memory>
#include <vector>

class SceneContext
{
public:
    SceneContext();
    ~SceneContext() = default;

    void addPrimitive(int type);

    void addCustomSolid(const char* name, int sides, float bottomRadius, float topRadius, float height);

    void addObject(SceneObject& object);
    void removeObject(uint32_t id);

    bool saveToFile(const std::string& filePath) const;
    bool loadFromFile(const std::string& filePath);
    void replaceWith(SceneSaveData&& saveData);
    void clear();

    Scene& getScene();
    const Scene& getScene() const;

private:
    Scene m_Scene;

    std::vector<std::unique_ptr<Mesh>> m_Meshes;
    std::vector<std::unique_ptr<SceneObject>> m_Objects;
};