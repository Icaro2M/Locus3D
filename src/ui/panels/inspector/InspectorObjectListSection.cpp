#include "InspectorObjectListSection.h"

#include <cstring>
#include <string>

#include <imgui.h>

#include "../../../application/AppEventBus.h"
#include "../../bridge/UIContext.h"

namespace
{
    void DrawSectionTitle(const char* title)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.64f, 0.76f, 1.0f));
        ImGui::Text("%s", title);
        ImGui::PopStyleColor();
    }

    void DrawMutedText(const char* text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.42f, 0.44f, 0.50f, 1.0f));
        ImGui::Text("%s", text);
        ImGui::PopStyleColor();
    }
}

InspectorObjectListSection::InspectorObjectListSection(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus),
    m_context(context),
    m_renamingObjectId(0)
{
    std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
}

void InspectorObjectListSection::draw(const InspectorState& state)
{
    DrawSectionTitle("OBJECTS");
    ImGui::Spacing();

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.095f, 0.100f, 0.118f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.17f, 0.18f, 0.22f, 1.0f));

    if (ImGui::BeginChild("InspectorObjectList", ImVec2(0.0f, 210.0f), true))
    {
        if (state.objects.empty())
        {
            ImGui::Dummy(ImVec2(0.0f, 8.0f));
            DrawMutedText("Empty");
        }
        else
        {
            for (const auto& object : state.objects)
            {
                drawObjectRow(object);
            }
        }
    }

    ImGui::EndChild();

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);

    ImGui::Spacing();

    const int objectCount = static_cast<int>(state.objects.size());

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.44f, 0.47f, 0.55f, 1.0f));

    if (objectCount == 1)
    {
        ImGui::Text("1 object");
    }
    else
    {
        ImGui::Text("%d objects", objectCount);
    }

    ImGui::PopStyleColor();
}

void InspectorObjectListSection::drawObjectRow(const InspectorObjectItem& object)
{
    ImGui::PushID(object.id);

    const bool isSelected = object.selected;
    const bool isRenaming = m_renamingObjectId == object.id;

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float deleteButtonWidth = 28.0f;
    const float rowHeight = 28.0f;
    const float nameWidth = availableWidth - deleteButtonWidth - 4.0f;

    if (isRenaming)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 5.0f));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.13f, 0.14f, 0.17f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.17f, 0.21f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.90f, 0.93f, 0.98f, 1.0f));

        ImGui::SetNextItemWidth(nameWidth);

        if (ImGui::InputText(
            "##RenameInput",
            m_renameBuffer,
            sizeof(m_renameBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue))
        {
            commitRename();
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar();

        ImGui::SameLine();
        ImGui::InvisibleButton("##DeletePlaceholder", ImVec2(deleteButtonWidth, rowHeight));

        ImGui::PopID();
        return;
    }

    if (isSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.12f, 0.30f, 0.58f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.36f, 0.68f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.18f, 0.40f, 0.76f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.96f, 1.00f, 1.0f));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.11f, 0.12f, 0.14f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.15f, 0.17f, 0.21f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.80f, 0.83f, 0.90f, 1.0f));
    }

    if (ImGui::Selectable(object.name.c_str(), isSelected, ImGuiSelectableFlags_AllowOverlap, ImVec2(nameWidth, rowHeight)))
    {
        m_context->selectedObjectId = object.id;
    }

    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        beginRename(object);
    }

    ImGui::PopStyleColor(4);

    ImGui::SameLine();

    if (isSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.18f, 0.20f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.78f, 0.24f, 0.26f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.90f, 0.30f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 0.92f, 0.92f, 1.0f));

        if (ImGui::Button("X", ImVec2(deleteButtonWidth, rowHeight)))
        {
            if (m_renamingObjectId == object.id)
            {
                m_renamingObjectId = 0;
                std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
            }

            m_context->selectedObjectId = 0;
            m_eventBus->emit(EventType::DeleteObject, object.id);
        }

        ImGui::PopStyleColor(4);
    }
    else
    {
        ImGui::InvisibleButton("##DeletePlaceholder", ImVec2(deleteButtonWidth, rowHeight));
    }

    ImGui::PopID();
}

void InspectorObjectListSection::beginRename(const InspectorObjectItem& object)
{
    m_context->selectedObjectId = object.id;
    m_renamingObjectId = object.id;

    std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
    std::strncpy(m_renameBuffer, object.name.c_str(), sizeof(m_renameBuffer) - 1);
}

void InspectorObjectListSection::commitRename()
{
    if (m_renamingObjectId == 0)
    {
        return;
    }

    std::string newName = std::string(m_renameBuffer);

    if (newName.empty())
    {
        m_renamingObjectId = 0;
        std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));
        return;
    }

    int renamedObjectId = m_renamingObjectId;

    for (auto& object : m_context->sceneObjects)
    {
        if (object.id == renamedObjectId)
        {
            object.name = newName;
            break;
        }
    }

    m_renamingObjectId = 0;
    std::memset(m_renameBuffer, 0, sizeof(m_renameBuffer));

    m_eventBus->emit(EventType::RenameObject, renamedObjectId);
}