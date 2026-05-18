#include "ViewportOverlay.h"
#include <imgui.h>

#include "../layout/UILayoutConstants.h"

ViewportOverlay::ViewportOverlay(UIContext* context)
    : m_context(context)
{
}

void ViewportOverlay::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const float titleBarHeight = ui::layout::GetTitleBarHeight(viewport->WorkSize.y);
    const float mainToolbarHeight = ui::layout::GetMainToolbarHeight(viewport->WorkSize.y);
    
    ImGui::SetNextWindowPos(ImVec2(
        viewport->WorkPos.x + ui::layout::ViewportOverlayLeftMargin,
        viewport->WorkPos.y +
        titleBarHeight +
        mainToolbarHeight +
        ui::layout::ToolbarSpacing +
        48.0f
    ));

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
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "ESC - Cancelar/Sair");
    }
    
    ImGui::End();
}
