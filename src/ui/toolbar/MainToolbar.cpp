#include "MainToolbar.h"
#include <imgui.h>

MainToolbar::MainToolbar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), m_context(context)
{
}

void MainToolbar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 50.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

    if (ImGui::Begin("MainToolbar", nullptr, flags))
    {
        if (m_context->activeToolId == 0) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Selecao", ImVec2(80, 30)))
        {
            m_context->activeToolId = 0;
            m_context->showCustomSolidPanel = false;
        }
        if (m_context->activeToolId == 0) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (m_context->activeToolId == 1) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Extrusao", ImVec2(80, 30)))
        {
            m_context->activeToolId = 1;
            m_context->showCustomSolidPanel = false;
        }
        if (m_context->activeToolId == 1) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (m_context->activeToolId == 2) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Mover Face", ImVec2(90, 30)))
        {
            m_context->activeToolId = 2;
            m_context->showCustomSolidPanel = false;
        }
        if (m_context->activeToolId == 2) ImGui::PopStyleColor();

        ImGui::SameLine();

        if (m_context->activeToolId == 3) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Escala Face", ImVec2(90, 30)))
        {
            m_context->activeToolId = 3;
            m_context->showCustomSolidPanel = false;
        }
        if (m_context->activeToolId == 3) ImGui::PopStyleColor();

        ImGui::SameLine(ImGui::GetWindowWidth() - 160.0f);

        if (m_context->showCustomSolidPanel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
        if (ImGui::Button("Solido Personalizado", ImVec2(150, 30)))
        {
            m_context->showCustomSolidPanel = !m_context->showCustomSolidPanel;
        }
        if (m_context->showCustomSolidPanel) ImGui::PopStyleColor();
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}