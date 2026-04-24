#include "ViewportOverlay.h"
#include <imgui.h>

ViewportOverlay::ViewportOverlay(UIContext* context)
    : m_context(context)
{
}

void ViewportOverlay::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + 10.0f, viewport->Pos.y + 120.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | 
                             ImGuiWindowFlags_NoSavedSettings | 
                             ImGuiWindowFlags_NoFocusOnAppearing | 
                             ImGuiWindowFlags_NoNav | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoBackground | 
                             ImGuiWindowFlags_NoInputs;

    if (ImGui::Begin("ViewportOverlay", nullptr, flags))
    {
        if (m_context->isFaceModeActive)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "[FACE MODE ATIVO]");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "[OBJECT MODE]");
        }

        ImGui::Dummy(ImVec2(0.0f, 10.0f));
        
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Atalhos:");
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "F - Alternar Modo");
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "ESC - Cancelar/Sair");
    }
    
    ImGui::End();
}