#pragma once

#include "../../application/AppEventBus.h"
#include "../UIContext.h"
#include <imgui.h>

enum class ToolbarIcon {
    SelectFace,
    ExtrudeFace,
    MoveFace,
    ScaleFace,
    Cube,
    Sphere,
    Cone,
    Cylinder,
    CustomSolid
};

class MainToolbar
{
public:
    MainToolbar(AppEventBus* eventBus, UIContext* context);
    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;

    // Helper devolvido para aceitar o texto (label) embaixo do ícone
    bool iconButton(const char* id, const char* label, ToolbarIcon icon, ImVec2 size, bool active);
    void drawToolbarIcon(ImDrawList* drawList, ToolbarIcon icon, ImVec2 center, float size, ImU32 color);
};