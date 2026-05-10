#pragma once

#include "io/SceneSaveData.h"
#include "scene/Scene.h"

#include <string>

class SceneSerializer
{
public:
    static bool save(const Scene& scene, const std::string& filePath);
    static SceneSaveData load(const std::string& filePath);
};