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
    
    // A nossa MainToolbar de botões grandes ocupa 100px agora.
    // Então o Inspector começa logo abaixo dela (Y = 105.0f para dar 5px de folga).
    float startY = viewport->Pos.y + 105.0f;
    
    // Calcula a altura dividindo a tela certinho
    float transformHeight = (viewport->Size.y - 105.0f) / 2.0f;
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 300.0f, startY));
    ImGui::SetNextWindowSize(ImVec2(300.0f, transformHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("Inspector", nullptr, flags)) 
    {
        // =========================================================================
        // A MÁGICA ESTÁ AQUI: Altura FIXA para a lista, em vez de usar (-35).
        // Isso permite que o Transform e a barra de renomear caibam perfeitamente.
        // =========================================================================
        float listHeight = transformHeight - 75.0f; // Tira o espaço do título e da barra de texto
        
        // Estilo de fundo mais escuro igual ao Mockup
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.13f, 0.15f, 1.0f));
        
        if (ImGui::BeginChild("ObjectList", ImVec2(0, listHeight), true))
        {
            for (auto& obj : m_context->sceneObjects)
            {
                ImGui::PushID(obj.id);

                bool isSelected = (m_context->selectedObjectId == obj.id);
                float availableWidth = ImGui::GetContentRegionAvail().x;
                float buttonSpace = 30.0f; 

                // Cor de fundo azul claro quando selecionado (Mockup)
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.3f, 0.5f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.25f, 0.35f, 0.55f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.3f, 0.4f, 0.6f, 1.0f));
                }

                // Desenha o item selecionável
                if (ImGui::Selectable(obj.name.c_str(), isSelected, ImGuiSelectableFlags_AllowOverlap, ImVec2(availableWidth - buttonSpace, 24.0f)))                {
                    m_context->selectedObjectId = obj.id;
                    std::strncpy(m_renameBuffer, obj.name.c_str(), sizeof(m_renameBuffer) - 1);
                }
                
                if (isSelected) {
                    ImGui::PopStyleColor(3);
                }

                // Posiciona o botão de exclusão
                ImGui::SameLine(ImGui::GetWindowWidth() - 35.0f);
                
                // Botão de deletar vermelho quando o item está selecionado
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 0.8f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                }

                // Lixeirinha (X)
                if (ImGui::Button("X", ImVec2(24.0f, 24.0f)))
                {
                    if (isSelected) {
                        m_context->selectedObjectId = 0;
                        std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
                    }
                    m_eventBus->emit(EventType::DeleteObject, static_cast<int>(obj.id));
                }

                if (isSelected) {
                    ImGui::PopStyleColor(4); 
                }

                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor(); // Restaura o fundo padrão do ImGui

        ImGui::Spacing();

        // Área de renomear o objeto (Aparece logo abaixo da lista)
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