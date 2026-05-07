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
    
    // Calcula o espaço que sobra acima do Transform Panel
    float transformHeight = (viewport->Size.y - 50.0f) / 2.0f;
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 300.0f, viewport->Pos.y + 50.0f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, transformHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("Inspector", nullptr, flags)) // Mudei o nome para inglês como no mockup
    {
        if (ImGui::BeginChild("ObjectList", ImVec2(0, -35), true))
        {
            for (auto& obj : m_context->sceneObjects)
            {
                ImGui::PushID(obj.id);

                bool isSelected = (m_context->selectedObjectId == obj.id);
                float availableWidth = ImGui::GetContentRegionAvail().x;
                float buttonSpace = 30.0f; 

                // Desenha o item selecionável
                if (ImGui::Selectable(obj.name.c_str(), isSelected, 0, ImVec2(availableWidth - buttonSpace, 24.0f)))
                {
                    m_context->selectedObjectId = obj.id;
                    std::strncpy(m_renameBuffer, obj.name.c_str(), sizeof(m_renameBuffer) - 1);
                }

                // Posiciona o botão de exclusão
                ImGui::SameLine(ImGui::GetWindowWidth() - 35.0f);
                
                // Pinta o botão de vermelho se o item estiver selecionado (como na referência)
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                }

                // Usamos "X" por enquanto. Para o ícone de lixeira exato, precisaríamos carregar uma fonte de ícones (FontAwesome).
                if (ImGui::Button("X", ImVec2(24.0f, 24.0f)))
                {
                    if (isSelected) {
                        m_context->selectedObjectId = 0;
                        std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
                    }
                    m_eventBus->emit(EventType::DeleteObject, static_cast<int>(obj.id));
                }

                if (isSelected) {
                    ImGui::PopStyleColor(4); // Remove as cores vermelhas para não vazar pros outros botões
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