#include "GizmoToolbar.h"
#include <imgui.h>

GizmoToolbar::GizmoToolbar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), m_context(context)
{
}

void GizmoToolbar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 15.0f, viewport->Pos.y + 80.0f));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_AlwaysAutoResize | 
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    if (ImGui::Begin("GizmoToolbar", nullptr, flags))
    {
        ImVec2 gizmoBtnSize(100.0f, 35.0f);

        bool isTranslateActive = (m_context->activeTransformMode == TransformMode::Translate);
        if (isTranslateActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.30f, 0.45f, 1.0f));
        if (ImGui::Button("Mover (W)", gizmoBtnSize))
        {
            m_context->activeTransformMode = TransformMode::Translate;
            m_eventBus->emit(EventType::InputKeyW, 0);
        }
        if (isTranslateActive) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isRotateActive = (m_context->activeTransformMode == TransformMode::Rotate);
        if (isRotateActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.30f, 0.45f, 1.0f));
        if (ImGui::Button("Girar (E)", gizmoBtnSize))
        {
            m_context->activeTransformMode = TransformMode::Rotate;
            m_eventBus->emit(EventType::InputKeyE, 0);
        }
        if (isRotateActive) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isScaleActive = (m_context->activeTransformMode == TransformMode::Scale);
        if (isScaleActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.30f, 0.45f, 1.0f));
        if (ImGui::Button("Escala (R)", gizmoBtnSize))
        {
            m_context->activeTransformMode = TransformMode::Scale;
            m_eventBus->emit(EventType::InputKeyR, 0);
        }
        if (isScaleActive) ImGui::PopStyleColor();
    }
    
    ImGui::End();
    ImGui::PopStyleVar(2);
}