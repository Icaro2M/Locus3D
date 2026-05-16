#include "InspectorTransformSection.h"

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

    void DrawAxisLabel(const char* label, const ImVec4& color)
    {
        ImGui::AlignTextToFramePadding();
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("%s", label);
        ImGui::PopStyleColor();
    }
}

InspectorTransformSection::InspectorTransformSection(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus),
    m_context(context)
{
}

void InspectorTransformSection::draw(const InspectorState& state)
{
    DrawSectionTitle("TRANSFORM");

    if (state.hasSelection)
    {
        const float resetButtonWidth = 46.0f;

        ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - resetButtonWidth);

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 3.0f));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.13f, 0.14f, 0.17f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.25f, 0.32f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.62f, 0.67f, 0.78f, 1.0f));

        if (ImGui::Button("Reset", ImVec2(resetButtonWidth, 22.0f)))
        {
            m_context->position[0] = 0.0f;
            m_context->position[1] = 0.0f;
            m_context->position[2] = 0.0f;

            m_context->rotation[0] = 0.0f;
            m_context->rotation[1] = 0.0f;
            m_context->rotation[2] = 0.0f;

            m_context->scale[0] = 1.0f;
            m_context->scale[1] = 1.0f;
            m_context->scale[2] = 1.0f;

            m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
        }

        ImGui::PopStyleColor(4);
        ImGui::PopStyleVar(2);
    }

    ImGui::Spacing();

    if (!state.hasSelection)
    {
        DrawMutedText("No selection");
        return;
    }

    if (drawVec3Control("POSITION", m_context->position))
    {
        m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
    }

    if (drawVec3Control("ROTATION", m_context->rotation))
    {
        m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
    }

    if (drawVec3Control("SCALE", m_context->scale))
    {
        m_eventBus->emit(EventType::TransformChanged, m_context->selectedObjectId);
    }
}

bool InspectorTransformSection::drawVec3Control(const char* label, float* values)
{
    bool changed = false;

    ImGui::PushID(label);

    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.68f, 0.72f, 0.80f, 1.0f));
    ImGui::Text("%s", label);
    ImGui::PopStyleColor();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(7.0f, 5.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5.0f, 6.0f));

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.13f, 0.14f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.16f, 0.17f, 0.21f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.20f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.86f, 0.89f, 0.95f, 1.0f));

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float labelWidth = 14.0f;
    const float spacing = 9.0f;
    const float inputWidth = (availableWidth - (labelWidth * 3.0f) - (spacing * 2.0f) - 24.0f) / 3.0f;

    DrawAxisLabel("X", ImVec4(0.95f, 0.30f, 0.34f, 1.0f));
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::PushItemWidth(inputWidth);

    if (ImGui::DragFloat("##X", &values[0], 0.1f, 0.0f, 0.0f, "%.3f"))
    {
        changed = true;
    }

    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, spacing);

    DrawAxisLabel("Y", ImVec4(0.35f, 0.85f, 0.42f, 1.0f));
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::PushItemWidth(inputWidth);

    if (ImGui::DragFloat("##Y", &values[1], 0.1f, 0.0f, 0.0f, "%.3f"))
    {
        changed = true;
    }

    ImGui::PopItemWidth();

    ImGui::SameLine(0.0f, spacing);

    DrawAxisLabel("Z", ImVec4(0.32f, 0.50f, 0.95f, 1.0f));
    ImGui::SameLine(0.0f, 4.0f);
    ImGui::PushItemWidth(inputWidth);

    if (ImGui::DragFloat("##Z", &values[2], 0.1f, 0.0f, 0.0f, "%.3f"))
    {
        changed = true;
    }

    ImGui::PopItemWidth();

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(3);

    ImGui::PopID();

    return changed;
}