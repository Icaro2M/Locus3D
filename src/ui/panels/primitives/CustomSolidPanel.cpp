#include "CustomSolidPanel.h"
#include <imgui.h>
#include <cstring>

CustomSolidPanel::CustomSolidPanel(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), 
      m_context(context),
      m_sides(5),
      m_bottomRadius(2.5f),
      m_topRadius(2.0f),
      m_height(3.0f)
{
    std::strncpy(m_nameBuffer, "solido", sizeof(m_nameBuffer) - 1);
}

void CustomSolidPanel::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 300.0f, viewport->Pos.y + 50.0f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, viewport->Size.y - 50.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("SolidoPersonalizadoContainer", nullptr, flags))
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Configurar Solido");
        ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
        if (ImGui::Button("X"))
        {
            m_context->showCustomSolidPanel = false;
        }

        ImGui::Spacing();
        
        if (ImGui::BeginChild("PreviewArea", ImVec2(0, 120), true))
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Preview Visual");
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::InputText("Nome", m_nameBuffer, sizeof(m_nameBuffer));
        
        if (ImGui::InputInt("Lados", &m_sides))
        {
            if (m_sides < 3) m_sides = 3;
        }
        
        ImGui::DragFloat("Raio Inf.", &m_bottomRadius, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Raio Sup.", &m_topRadius, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Altura", &m_height, 0.1f, 0.1f, 100.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Adicionar", ImVec2(-1, 40)))
        {
            m_eventBus->emit(EventType::AddCustomSolid, m_sides);
            m_context->showCustomSolidPanel = false;
        }
    }
    ImGui::End();
}