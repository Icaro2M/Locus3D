#include "TransformPanel.h"
#include <imgui.h>

TransformPanel::TransformPanel(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), m_context(context)
{
}

void TransformPanel::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    float panelHeight = (viewport->Size.y - 50.0f) / 2.0f;
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 300.0f, viewport->Pos.y + 50.0f + panelHeight));
    ImGui::SetNextWindowSize(ImVec2(300.0f, panelHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove;

    if (ImGui::Begin("Transform", nullptr, flags))
    {
        if (m_context->selectedObjectId == 0)
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Nenhum objeto selecionado");
        }
        else
        {
            ImGui::Text("ID: %u", m_context->selectedObjectId);
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::DragFloat3("Posicao", m_context->position, 0.1f))
            {
                m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
            }

            if (ImGui::DragFloat3("Rotacao", m_context->rotation, 1.0f))
            {
                m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
            }

            if (ImGui::DragFloat3("Escala", m_context->scale, 0.1f))
            {
                m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Resetar Transform", ImVec2(-1, 0)))
            {
                m_context->position[0] = m_context->position[1] = m_context->position[2] = 0.0f;
                m_context->rotation[0] = m_context->rotation[1] = m_context->rotation[2] = 0.0f;
                m_context->scale[0] = m_context->scale[1] = m_context->scale[2] = 1.0f;
                m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
            }
        }
    }
    ImGui::End();
}