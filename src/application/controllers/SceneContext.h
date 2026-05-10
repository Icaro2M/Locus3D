#pragma once

#include "../../scene/Scene.h"
#include "../../scene/SceneObject.h"
#include "../../geometry/Mesh.h"
#include "../../geometry/MeshFaceData.h"
#include "../../geometry/primitives/PrimitiveFactory.h"
#include "../../io/SceneSaveData.h"

#include <glm/glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

class SceneContext {
public:
    SceneContext();
    ~SceneContext() = default;

    void addPrimitive(int type);
    void addCustomSolid(const char* name, int sides, float bottomRadius, float topRadius, float height);

    SceneObject* createObjectFromMeshData(
        const std::string& name,
        const std::vector<float>& vertices,
        const std::vector<unsigned int>& indices,
        const std::vector<LogicalFace>& logicalFaces,
        const glm::vec3& position,
        const glm::vec3& rotation,
        const glm::vec3& scale
    );

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