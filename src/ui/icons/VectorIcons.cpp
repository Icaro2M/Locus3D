#include "VectorIcons.h"

namespace ui::icons
{
    static float LineWidth(float size)
    {
        return size * 0.105f;
    }

    void DrawSelect(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        ImVec2 points[7] = {
            ImVec2(center.x - s * 0.45f, center.y - s * 0.80f),
            ImVec2(center.x - s * 0.45f, center.y + s * 0.75f),
            ImVec2(center.x - s * 0.08f, center.y + s * 0.42f),
            ImVec2(center.x + s * 0.28f, center.y + s * 0.82f),
            ImVec2(center.x + s * 0.52f, center.y + s * 0.58f),
            ImVec2(center.x + s * 0.15f, center.y + s * 0.20f),
            ImVec2(center.x + s * 0.62f, center.y + s * 0.02f)
        };

        drawList->AddPolyline(points, 7, color, ImDrawFlags_Closed, lw);
    }

    void DrawTranslate(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        drawList->AddLine(ImVec2(center.x, center.y + s * 0.65f), ImVec2(center.x, center.y - s * 0.75f), color, lw);
        drawList->AddLine(ImVec2(center.x - s * 0.65f, center.y + s * 0.65f), ImVec2(center.x + s * 0.75f, center.y + s * 0.65f), color, lw);

        drawList->AddTriangleFilled(
            ImVec2(center.x, center.y - s),
            ImVec2(center.x - s * 0.25f, center.y - s * 0.62f),
            ImVec2(center.x + s * 0.25f, center.y - s * 0.62f),
            color
        );

        drawList->AddTriangleFilled(
            ImVec2(center.x + s, center.y + s * 0.65f),
            ImVec2(center.x + s * 0.62f, center.y + s * 0.40f),
            ImVec2(center.x + s * 0.62f, center.y + s * 0.90f),
            color
        );
    }

    void DrawRotate(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        drawList->PathArcTo(center, s * 0.82f, -2.70f, 1.30f, 28);
        drawList->PathStroke(color, 0, lw);

        drawList->AddTriangleFilled(
            ImVec2(center.x + s * 0.52f, center.y - s * 0.57f),
            ImVec2(center.x + s * 0.90f, center.y - s * 0.45f),
            ImVec2(center.x + s * 0.65f, center.y - s * 0.18f),
            color
        );
    }

    void DrawScale(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        ImVec2 min = ImVec2(center.x - s * 0.65f, center.y - s * 0.65f);
        ImVec2 max = ImVec2(center.x + s * 0.65f, center.y + s * 0.65f);

        drawList->AddRect(min, max, color, 0.0f, 0, lw);

        drawList->AddLine(ImVec2(center.x - s * 0.95f, center.y + s * 0.95f), ImVec2(center.x - s * 0.45f, center.y + s * 0.45f), color, lw);
        drawList->AddLine(ImVec2(center.x + s * 0.95f, center.y - s * 0.95f), ImVec2(center.x + s * 0.45f, center.y - s * 0.45f), color, lw);

        drawList->AddTriangleFilled(
            ImVec2(center.x + s * 0.95f, center.y - s * 0.95f),
            ImVec2(center.x + s * 0.55f, center.y - s * 0.88f),
            ImVec2(center.x + s * 0.88f, center.y - s * 0.55f),
            color
        );
    }

    void DrawExtrudeFace(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        ImVec2 bottomMin = ImVec2(center.x - s * 0.60f, center.y + s * 0.08f);
        ImVec2 bottomMax = ImVec2(center.x + s * 0.60f, center.y + s * 0.70f);

        ImVec2 topMin = ImVec2(center.x - s * 0.48f, center.y - s * 0.72f);
        ImVec2 topMax = ImVec2(center.x + s * 0.48f, center.y - s * 0.18f);

        drawList->AddRect(bottomMin, bottomMax, color, 0.0f, 0, lw);
        drawList->AddRect(topMin, topMax, color, 0.0f, 0, lw);

        drawList->AddLine(ImVec2(center.x, topMax.y), ImVec2(center.x, bottomMin.y), color, lw);
    }

    void DrawMoveFace(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        DrawTranslate(drawList, center, size, color);
    }

    void DrawScaleFace(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        DrawScale(drawList, center, size, color);
    }

    void DrawCube(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);
        float o = s * 0.32f;

        ImVec2 frontMin = ImVec2(center.x - s * 0.55f, center.y - s * 0.35f);
        ImVec2 frontMax = ImVec2(center.x + s * 0.35f, center.y + s * 0.55f);

        ImVec2 backMin = ImVec2(frontMin.x + o, frontMin.y - o);
        ImVec2 backMax = ImVec2(frontMax.x + o, frontMax.y - o);

        drawList->AddRect(backMin, backMax, color, 0.0f, 0, lw);
        drawList->AddRect(frontMin, frontMax, color, 0.0f, 0, lw);

        drawList->AddLine(frontMin, backMin, color, lw);
        drawList->AddLine(ImVec2(frontMax.x, frontMin.y), ImVec2(backMax.x, backMin.y), color, lw);
        drawList->AddLine(ImVec2(frontMin.x, frontMax.y), ImVec2(backMin.x, backMax.y), color, lw);
        drawList->AddLine(frontMax, backMax, color, lw);
    }

    void DrawSphere(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        drawList->AddCircle(center, s * 0.86f, color, 32, lw);
        drawList->AddEllipse(center, ImVec2(s * 0.36f, s * 0.86f), color, 32, 0.0f, lw * 0.75f);
        drawList->AddEllipse(center, ImVec2(s * 0.86f, s * 0.30f), color, 32, 0.0f, lw * 0.75f);
    }

    void DrawCylinder(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        ImVec2 top = ImVec2(center.x, center.y - s * 0.55f);
        ImVec2 bottom = ImVec2(center.x, center.y + s * 0.55f);

        drawList->AddEllipse(top, ImVec2(s * 0.58f, s * 0.22f), color, 32, 0.0f, lw);
        drawList->AddLine(ImVec2(center.x - s * 0.58f, top.y), ImVec2(center.x - s * 0.58f, bottom.y), color, lw);
        drawList->AddLine(ImVec2(center.x + s * 0.58f, top.y), ImVec2(center.x + s * 0.58f, bottom.y), color, lw);
        drawList->AddEllipse(bottom, ImVec2(s * 0.58f, s * 0.22f), color, 32, 0.0f, lw);
    }

    void DrawCone(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        ImVec2 top = ImVec2(center.x, center.y - s * 0.82f);
        ImVec2 left = ImVec2(center.x - s * 0.70f, center.y + s * 0.55f);
        ImVec2 right = ImVec2(center.x + s * 0.70f, center.y + s * 0.55f);

        drawList->AddLine(top, left, color, lw);
        drawList->AddLine(top, right, color, lw);
        drawList->AddEllipse(ImVec2(center.x, center.y + s * 0.55f), ImVec2(s * 0.70f, s * 0.22f), color, 32, 0.0f, lw);
    }

    void DrawCustomSolid(ImDrawList* drawList, ImVec2 center, float size, ImU32 color)
    {
        float s = size * 0.5f;
        float lw = LineWidth(size);

        ImVec2 points[5] = {
            ImVec2(center.x, center.y - s * 0.88f),
            ImVec2(center.x + s * 0.82f, center.y - s * 0.20f),
            ImVec2(center.x + s * 0.52f, center.y + s * 0.78f),
            ImVec2(center.x - s * 0.52f, center.y + s * 0.78f),
            ImVec2(center.x - s * 0.82f, center.y - s * 0.20f)
        };

        drawList->AddPolyline(points, 5, color, ImDrawFlags_Closed, lw);
    }
}