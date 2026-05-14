#include "InspectorPanel.h"

#include <imgui.h>

#include "../../bridge/UIContext.h"

namespace
{
    void DrawInspectorHeader()
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.80f, 0.92f, 1.0f));
        ImGui::Text("INSPECTOR");
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
}

InspectorPanel::InspectorPanel(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus),
    m_context(context),
    m_objectListSection(eventBus, context),
    m_transformSection(eventBus, context)
{
}

void InspectorPanel::draw()
{
    InspectorState state = buildState();

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    const float panelWidth = 305.0f;
    const float startY = viewport->Pos.y + 105.0f;
    const float panelHeight = viewport->Size.y - 105.0f;

    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - panelWidth, startY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 7.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.075f, 0.078f, 0.090f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.17f, 0.18f, 0.22f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.18f, 0.19f, 0.23f, 1.0f));

    if (ImGui::Begin("Inspector", nullptr, flags))
    {
        DrawInspectorHeader();

        m_objectListSection.draw(state);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        m_transformSection.draw(state);
    }

    ImGui::End();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(4);
}

InspectorState InspectorPanel::buildState() const
{
    InspectorState state;

    state.selectedObjectId = m_context->selectedObjectId;
    state.hasSelection = m_context->selectedObjectId != 0;

    state.transform.position = glm::vec3(
        m_context->position[0],
        m_context->position[1],
        m_context->position[2]
    );

    state.transform.rotation = glm::vec3(
        m_context->rotation[0],
        m_context->rotation[1],
        m_context->rotation[2]
    );

    state.transform.scale = glm::vec3(
        m_context->scale[0],
        m_context->scale[1],
        m_context->scale[2]
    );

    for (const auto& object : m_context->sceneObjects)
    {
        InspectorObjectItem item;
        item.id = object.id;
        item.name = object.name;
        item.selected = object.id == m_context->selectedObjectId;

        if (item.selected)
        {
            state.selectedObjectName = item.name;
        }

        state.objects.push_back(item);
    }

    return state;
}