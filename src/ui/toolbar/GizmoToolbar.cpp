#include "GizmoToolbar.h"

#include <imgui.h>

#include "../../resources/AssetPaths.h"
#include "../widgets/buttons/ToggleIconButton.h"

GizmoToolbar::GizmoToolbar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus),
    m_context(context)
{
}

void GizmoToolbar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 10.0f, viewport->Pos.y + 78.0f));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6.0f, 0.0f));

    if (ImGui::Begin("GizmoToolbar", nullptr, flags))
    {
        drawBackground();

        bool isTranslateActive = m_context->activeTransformMode == TransformMode::Translate;

        if (ui::ToggleIconButton({
            "gizmo_toolbar_translate",
            "Mover gizmo (W)",
            isTranslateActive,
            true,
            AssetPaths::toolbarIcon("translate.png")
            }))
        {
            m_context->activeTransformMode = TransformMode::Translate;
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::InputKeyW, 0);
        }

        ImGui::SameLine();

        bool isRotateActive = m_context->activeTransformMode == TransformMode::Rotate;

        if (ui::ToggleIconButton({
            "gizmo_toolbar_rotate",
            "Rotacionar gizmo (E)",
            isRotateActive,
            true,
            AssetPaths::toolbarIcon("rotate.png")
            }))
        {
            m_context->activeTransformMode = TransformMode::Rotate;
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::InputKeyE, 0);
        }

        ImGui::SameLine();

        bool isScaleActive = m_context->activeTransformMode == TransformMode::Scale;

        if (ui::ToggleIconButton({
            "gizmo_toolbar_scale",
            "Escalar gizmo (R)",
            isScaleActive,
            true,
            AssetPaths::toolbarIcon("scale.png")
            }))
        {
            m_context->activeTransformMode = TransformMode::Scale;
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::InputKeyR, 0);
        }
    }

    ImGui::End();

    ImGui::PopStyleVar(3);
}

void GizmoToolbar::drawBackground()
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 padding = ImVec2(8.0f, 8.0f);
    ImVec2 buttonSize = ImVec2(38.0f, 38.0f);
    float spacing = 6.0f;

    float width = padding.x * 2.0f + buttonSize.x * 3.0f + spacing * 2.0f;
    float height = padding.y * 2.0f + buttonSize.y;

    drawList->AddRectFilled(
        pos,
        ImVec2(pos.x + width, pos.y + height),
        IM_COL32(18, 20, 24, 235),
        9.0f
    );

    drawList->AddRect(
        pos,
        ImVec2(pos.x + width, pos.y + height),
        IM_COL32(42, 46, 54, 255),
        9.0f,
        0,
        1.0f
    );

    ImGui::SetCursorScreenPos(ImVec2(pos.x + padding.x, pos.y + padding.y));
}