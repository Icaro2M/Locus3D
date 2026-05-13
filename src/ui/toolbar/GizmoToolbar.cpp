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

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 10.0f, viewport->Pos.y + 76.0f));

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
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 0.0f));

    if (ImGui::Begin("GizmoToolbar", nullptr, flags))
    {
        ui::ButtonVisualStyle gizmoButtonStyle;
        gizmoButtonStyle.size = ImVec2(50.0f, 50.0f);
        gizmoButtonStyle.rounding = 11.0f;
        gizmoButtonStyle.borderThickness = 1.0f;
        gizmoButtonStyle.iconScale = 0.56f;

        ui::ButtonVisualStyle translateButtonStyle = gizmoButtonStyle;
        translateButtonStyle.iconScale = 0.72f;

        bool isTranslateActive = m_context->activeTransformMode == TransformMode::Translate;

        if (ui::ToggleIconButton({
            "gizmo_toolbar_translate",
            "Mover gizmo (W)",
            isTranslateActive,
            true,
            AssetPaths::toolbarIcon("translate.png"),
            translateButtonStyle
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
            AssetPaths::toolbarIcon("rotate.png"),
            gizmoButtonStyle
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
            AssetPaths::toolbarIcon("scale.png"),
            gizmoButtonStyle
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