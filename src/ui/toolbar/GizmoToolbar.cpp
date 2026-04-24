#include "GizmoToolbar.h"
#include <imgui.h>

GizmoToolbar::GizmoToolbar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), m_context(context)
{
}

void GizmoToolbar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 10.0f, viewport->Pos.y + 60.0f));
    ImGui::SetNextWindowSize(ImVec2(280.0f, 45.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 0.0f));

    if (ImGui::Begin("GizmoToolbar", nullptr, flags))
    {
        if (m_context->activeTransformMode == TransformMode::Translate) 
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Mover (W)", ImVec2(85, 30)))
        {
            m_context->activeTransformMode = TransformMode::Translate;
        }
        if (m_context->activeTransformMode == TransformMode::Translate) 
            ImGui::PopStyleColor();

        ImGui::SameLine();

        if (m_context->activeTransformMode == TransformMode::Rotate) 
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Girar (E)", ImVec2(85, 30)))
        {
            m_context->activeTransformMode = TransformMode::Rotate;
        }
        if (m_context->activeTransformMode == TransformMode::Rotate) 
            ImGui::PopStyleColor();

        ImGui::SameLine();

        if (m_context->activeTransformMode == TransformMode::Scale) 
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
        if (ImGui::Button("Escala (R)", ImVec2(85, 30)))
        {
            m_context->activeTransformMode = TransformMode::Scale;
        }
        if (m_context->activeTransformMode == TransformMode::Scale) 
            ImGui::PopStyleColor();
    }
    
    ImGui::End();
    ImGui::PopStyleVar(2);
}