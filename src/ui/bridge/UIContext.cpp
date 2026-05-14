#include "UIContext.h"

UIContext::UIContext()
    : selectedObjectId(0),
    isFaceModeActive(false),
    activeToolId(0),
    activeTransformMode(TransformMode::None),
    showCustomSolidPanel(false),
    customSolidSides(6),
    customSolidBottomRadius(1.0f),
    customSolidTopRadius(1.0f),
    customSolidHeight(2.0f)
{
    position[0] = 0.0f;
    position[1] = 0.0f;
    position[2] = 0.0f;

    rotation[0] = 0.0f;
    rotation[1] = 0.0f;
    rotation[2] = 0.0f;

    scale[0] = 1.0f;
    scale[1] = 1.0f;
    scale[2] = 1.0f;

    customSolidName[0] = '\0';
}

void UIContext::resetSelection()
{
    selectedObjectId = 0;

    for (auto& obj : sceneObjects)
    {
        obj.isSelected = false;
    }
}

void UIContext::clearScene()
{
    sceneObjects.clear();
    resetSelection();
}   