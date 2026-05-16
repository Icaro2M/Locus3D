#include "TopMenuBar.h"

#include <imgui.h>

TopMenuBar::TopMenuBar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus),
    m_context(context),
    m_fileMenu(eventBus)
{
}

void TopMenuBar::draw()
{
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12.0f, 4.0f));

    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.075f, 0.078f, 0.090f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.22f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.12f, 0.30f, 0.60f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.90f, 0.94f, 1.0f));

    if (ImGui::BeginMainMenuBar())
    {
        m_fileMenu.draw();

        ImGui::EndMainMenuBar();
    }

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(2);
}