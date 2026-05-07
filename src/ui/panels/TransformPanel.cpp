#include "TransformPanel.h"
#include <imgui.h>

// =====================================================================
// FUNÇÃO AUXILIAR: Cria o layout moderno de 3 eixos (X, Y, Z) lado a lado
// =====================================================================
static bool DrawModernVec3(const char* label, float* values)
{
    bool changed = false;
    ImGui::PushID(label);

    // Título do campo (ex: "Position")
    ImGui::Text("%s", label);
    ImGui::Spacing();

    // Calcula a largura de cada input para caberem os 3 na mesma linha perfeitamente
    float availableWidth = ImGui::GetContentRegionAvail().x;
    // Dividimos o espaço por 3, descontando a largura das letras X,Y,Z e os espaços entre eles
    float inputWidth = (availableWidth - 65.0f) / 3.0f; 

    // --- Eixo X (Vermelho) ---
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.9f, 0.2f, 0.2f, 1.0f), "X"); // Letra vermelha
    ImGui::SameLine(0, 5.0f);
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::DragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.3f")) changed = true;
    ImGui::PopItemWidth();

    ImGui::SameLine(0, 15.0f); // Espaço entre o X e o Y

    // --- Eixo Y (Verde) ---
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Y"); // Letra verde
    ImGui::SameLine(0, 5.0f);
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::DragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.3f")) changed = true;
    ImGui::PopItemWidth();

    ImGui::SameLine(0, 15.0f); // Espaço entre o Y e o Z

    // --- Eixo Z (Azul) ---
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(ImVec4(0.2f, 0.4f, 0.9f, 1.0f), "Z"); // Letra azul
    ImGui::SameLine(0, 5.0f);
    ImGui::PushItemWidth(inputWidth);
    if (ImGui::DragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.3f")) changed = true;
    ImGui::PopItemWidth();

    ImGui::PopID();
    
    // Espaço no final para respirar antes do próximo item (Rotation/Scale)
    ImGui::Dummy(ImVec2(0.0f, 5.0f));

    return changed;
}
// =====================================================================


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
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "ID: %u", m_context->selectedObjectId);
            ImGui::Separator();
            ImGui::Spacing();

            // Usamos a nossa nova função personalizada para cada componente!
            if (DrawModernVec3("Position", m_context->position))
            {
                m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
            }

            if (DrawModernVec3("Rotation", m_context->rotation))
            {
                m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
            }

            if (DrawModernVec3("Scale", m_context->scale))
            {
                m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Button("Resetar Transform", ImVec2(-1, 30))) // Aumentei a altura do botão para ficar mais elegante
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