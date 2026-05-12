#pragma once

#include <imgui.h>

namespace ui::icons
{
    void DrawSelect(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);

    void DrawTranslate(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
    void DrawRotate(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
    void DrawScale(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);

    void DrawExtrudeFace(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
    void DrawMoveFace(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
    void DrawScaleFace(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);

    void DrawCube(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
    void DrawSphere(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
    void DrawCylinder(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
    void DrawCone(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
    void DrawCustomSolid(ImDrawList* drawList, ImVec2 center, float size, ImU32 color);
}