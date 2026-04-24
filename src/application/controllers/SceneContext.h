#pragma once

#include "../../scene/Scene.h"
#include "../../scene/SceneObject.h"
#include "../../geometry/Mesh.h"
#include "../../geometry/primitives/PrimitiveFactory.h"

#include <memory>
#include <vector>

class SceneContext
{
public:
    SceneContext();
    ~SceneContext() = default;

    void addPrimitive(int type);
    void addObject(SceneObject& object);

    Scene& getScene();
    const Scene& getScene() const;

private:
    Scene m_Scene;

    std::vector<std::unique_ptr<Mesh>> m_Meshes;
    std::vector<std::unique_ptr<SceneObject>> m_Objects;

private:
    void addMeshAsObject(Mesh&& mesh);
};