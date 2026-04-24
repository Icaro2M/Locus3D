#include "InspectorPanel.h"
#include <imgui.h>
#include <cstring>
#include <string>

InspectorPanel::InspectorPanel(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), m_context(context)
{
    std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
}

void InspectorPanel::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 300.0f, viewport->Pos.y + 50.0f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, (viewport->Size.y - 50.0f) / 2.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("Inspetor", nullptr, flags))
    {
        if (ImGui::BeginChild("ObjectList", ImVec2(0, -35), true))
        {
            for (size_t i = 0; i < m_context->sceneObjects.size(); ++i)
            {
                auto& obj = m_context->sceneObjects[i];
                bool isSelected = (m_context->selectedObjectId == obj.id);

                std::string label = obj.name + "##" + std::to_string(obj.id);
                
                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    m_context->selectedObjectId = obj.id;
                    std::strncpy(m_renameBuffer, obj.name.c_str(), sizeof(m_renameBuffer) - 1);
                }

                if (isSelected)
                {
                    ImGui::SameLine(ImGui::GetWindowWidth() - 35.0f);
                    if (ImGui::Button(("X##del" + std::to_string(obj.id)).c_str()))
                    {
                        m_context->selectedObjectId = 0;
                        m_eventBus->emit(EventType::DeleteObject, obj.id);
                    }
                }
            }
        }
        ImGui::EndChild();

        if (m_context->selectedObjectId != 0)
        {
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::InputText("##rename", m_renameBuffer, sizeof(m_renameBuffer), ImGuiInputTextFlags_EnterReturnsTrue))
            {
                for (auto& obj : m_context->sceneObjects)
                {
                    if (obj.id == m_context->selectedObjectId)
                    {
                        obj.name = std::string(m_renameBuffer);
                        m_eventBus->emit(EventType::RenameObject, obj.id);
                        break;
                    }
                }
            }
        }
    }
    ImGui::End();
}