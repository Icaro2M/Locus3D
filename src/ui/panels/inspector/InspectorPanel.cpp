#include "InspectorPanel.h"

#include <imgui.h>

#include "../../bridge/UIContext.h"
#include "../../layout/UILayoutConstants.h"

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

    float ClampInspectorWidth(float width)
    {
        if (width < ui::layout::InspectorMinWidth)
        {
            return ui::layout::InspectorMinWidth;
        }

        if (width > ui::layout::InspectorMaxWidth)
        {
            return ui::layout::InspectorMaxWidth;
        }

        return width;
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

    if (!m_hasUserResized)
    {
        m_panelWidth = ClampInspectorWidth(viewport->Size.x * ui::layout::InspectorWidthRatio);
    }
    else
    {
        m_panelWidth = ClampInspectorWidth(m_panelWidth);
    }

    const float startY = viewport->Pos.y + ui::layout::MainToolbarHeight;
    const float panelHeight = viewport->Size.y - ui::layout::MainToolbarHeight;

    const ImVec2 panelPos(
        viewport->Pos.x + viewport->Size.x - m_panelWidth,
        startY
    );

    ImGui::SetNextWindowPos(panelPos);
    ImGui::SetNextWindowSize(ImVec2(m_panelWidth, panelHeight));

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
        ImVec2 contentCursor = ImGui::GetCursorScreenPos();

        drawResizeHandle(panelPos, panelHeight);

        ImGui::SetCursorScreenPos(contentCursor);

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

void InspectorPanel::drawResizeHandle(const ImVec2& panelPos, float panelHeight)
{
    const float handleWidth = ui::layout::InspectorResizeHandleWidth;

    ImGui::SetCursorScreenPos(ImVec2(panelPos.x, panelPos.y));

    ImGui::InvisibleButton("##InspectorResizeHandle", ImVec2(handleWidth, panelHeight));

    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();

    if (hovered || active)
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }

    if (active)
    {
        m_hasUserResized = true;
        m_panelWidth = ClampInspectorWidth(m_panelWidth - ImGui::GetIO().MouseDelta.x);
    }

    if (hovered || active)
    {
        ImU32 color = ImGui::GetColorU32(
            active
            ? ImVec4(0.25f, 0.55f, 1.0f, 1.0f)
            : ImVec4(0.30f, 0.34f, 0.42f, 1.0f)
        );

        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(panelPos.x, panelPos.y),
            ImVec2(panelPos.x + 2.0f, panelPos.y + panelHeight),
            color
        );
    }
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