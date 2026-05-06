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
        // Área da lista de objetos
        if (ImGui::BeginChild("ObjectList", ImVec2(0, -35), true))
        {
            for (auto& obj : m_context->sceneObjects)
            {
                ImGui::PushID(obj.id);

                bool isSelected = (m_context->selectedObjectId == obj.id);

                // A correção mágica: limitamos a largura do Selectable!
                // Usa o espaço disponível na janela menos 35 pixels (que é o espaço do botão X)
                float availableWidth = ImGui::GetContentRegionAvail().x;
                float buttonSpace = 35.0f; 

                // O ImVec2 no final obriga o Selectable a não invadir a área do botão
                if (ImGui::Selectable(obj.name.c_str(), isSelected, 0, ImVec2(availableWidth - buttonSpace, 0)))
                {
                    m_context->selectedObjectId = obj.id;
                    std::strncpy(m_renameBuffer, obj.name.c_str(), sizeof(m_renameBuffer) - 1);
                }

                ImGui::SameLine(ImGui::GetWindowWidth() - 35.0f);
                
                // Agora o botão está livre para receber o clique!
                if (ImGui::Button("X"))
                {
                    if (isSelected) {
                        m_context->selectedObjectId = 0;
                        std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
                    }
                    // Emite o evento com segurança de tipo (casting)
                    m_eventBus->emit(EventType::DeleteObject, static_cast<int>(obj.id));
                }

                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        // Área de renomear o objeto
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
                        m_eventBus->emit(EventType::RenameObject, static_cast<int>(obj.id));
                        break;
                    }
                }
            }
        }
    }
    ImGui::End();
}