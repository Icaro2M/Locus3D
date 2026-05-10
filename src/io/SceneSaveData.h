#pragma once

#include "scene/Scene.h"
#include "scene/SceneObject.h"
#include "geometry/Mesh.h"

#include <memory>
#include <vector>

class SceneSaveData
{
private:
    Scene m_Scene;
    std::vector<std::unique_ptr<Mesh>> m_Meshes;
    std::vector<std::unique_ptr<SceneObject>> m_Objects;

public:
    SceneSaveData() = default;

    SceneSaveData(const SceneSaveData&) = delete;
    SceneSaveData& operator=(const SceneSaveData&) = delete;

    SceneSaveData(SceneSaveData&&) noexcept = default;
    SceneSaveData& operator=(SceneSaveData&&) noexcept = default;

    Scene& getScene()
    {
        return m_Scene;
    }

    const Scene& getScene() const
    {
        return m_Scene;
    }

    bool isEmpty() const
    {
        return m_Objects.empty();
    }

    void addObject(std::unique_ptr<Mesh> mesh, std::unique_ptr<SceneObject> object)
    {
        m_Meshes.push_back(std::move(mesh));
        m_Objects.push_back(std::move(object));
        m_Scene.addObject(*m_Objects.back());
    }

    Scene takeScene()
    {
        return std::move(m_Scene);
    }

    std::vector<std::unique_ptr<Mesh>> takeMeshes()
    {
        return std::move(m_Meshes);
    }

    std::vector<std::unique_ptr<SceneObject>> takeObjects()
    {
        return std::move(m_Objects);
    }
};