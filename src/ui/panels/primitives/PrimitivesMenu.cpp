#include "PrimitivesMenu.h"
#include <imgui.h>

PrimitivesMenu::PrimitivesMenu(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), m_context(context)
{
}

void PrimitivesMenu::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 370.0f, viewport->Pos.y + 50.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(60.0f, 200.0f), ImGuiCond_FirstUseEver);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("Primitives", nullptr, flags))
    {
        if (ImGui::Button("Cub", ImVec2(45, 40)))
        {
            m_eventBus->emit(EventType::AddPrimitive, 0);
        }
        
        ImGui::Spacing();
        
        if (ImGui::Button("Esf", ImVec2(45, 40)))
        {
            m_eventBus->emit(EventType::AddPrimitive, 1);
        }
        
        ImGui::Spacing();
        
        if (ImGui::Button("Con", ImVec2(45, 40)))
        {
            m_eventBus->emit(EventType::AddPrimitive, 2);
        }
        
        ImGui::Spacing();
        
        if (ImGui::Button("Cil", ImVec2(45, 40)))
        {
            m_eventBus->emit(EventType::AddPrimitive, 3);
        }
    }
    ImGui::End();
}