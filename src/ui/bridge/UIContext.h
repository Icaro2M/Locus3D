#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "../../tools/transform/TransformTypes.h"

struct SceneObjectInfo {
    uint32_t id;
    std::string name;
    bool isSelected;
};

struct UIContext {
    std::vector<SceneObjectInfo> sceneObjects;
    uint32_t selectedObjectId;

    float position[3];
    float rotation[3];
    float scale[3];

    bool isFaceModeActive;
    int activeToolId;
    TransformMode activeTransformMode;
    
    bool showCustomSolidPanel;

    // =========================================================
    // VARIÁVEIS PARA O GERADOR DE SÓLIDO PERSONALIZADO
    // =========================================================
    char customSolidName[256];
    int customSolidSides;
    float customSolidBottomRadius;
    float customSolidTopRadius;
    float customSolidHeight;
    // =========================================================

    UIContext();

    void resetSelection();
    void clearScene();
};