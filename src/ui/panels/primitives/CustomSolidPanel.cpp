#include "CustomSolidPanel.h"

#include <algorithm>
#include <cstring>

#include <imgui.h>

#include "../../layout/RightSidePanelLayout.h"
#include "../../layout/UILayoutConstants.h"

namespace
{
    void PushPanelStyle()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, ui::layout::SidePanelPaddingY));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(ui::layout::SidePanelItemSpacingX, ui::layout::SidePanelItemSpacingY));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 7.0f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.075f, 0.078f, 0.090f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.17f, 0.18f, 0.22f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.18f, 0.19f, 0.23f, 1.0f));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.055f, 0.060f, 0.072f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.085f, 0.095f, 0.115f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.100f, 0.115f, 0.140f, 1.0f));

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.31f, 0.56f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.38f, 0.68f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.25f, 0.48f, 1.0f));
    }

    void PopPanelStyle()
    {
        ImGui::PopStyleColor(9);
        ImGui::PopStyleVar(5);
    }

    void DrawSectionTitle(const char* text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.67f, 0.80f, 1.0f));
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
    }

    void DrawFieldLabel(const char* text)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82f, 0.85f, 0.92f, 1.0f));
        ImGui::TextUnformatted(text);
        ImGui::PopStyleColor();
    }
}

CustomSolidPanel::CustomSolidPanel(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus),
    m_context(context)
{
}

void CustomSolidPanel::draw()
{
    ui::layout::RightSidePanelMetrics metrics =
        ui::layout::CalculateRightSidePanelMetrics(m_context);

    ImGui::SetNextWindowPos(metrics.position);
    ImGui::SetNextWindowSize(metrics.size);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings;

    PushPanelStyle();

    if (ImGui::Begin("CustomSolidPanel", nullptr, flags))
    {
        ImVec2 contentCursor = ImGui::GetCursorScreenPos();

        ui::layout::DrawRightSidePanelResizeHandle(m_context, metrics);

        ImGui::SetCursorScreenPos(contentCursor);

        drawHeader();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        drawPreviews();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        drawFields();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        drawActions();
    }

    ImGui::End();

    PopPanelStyle();
}

void CustomSolidPanel::drawHeader()
{
    ImGui::AlignTextToFramePadding();

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.74f, 0.80f, 0.92f, 1.0f));
    ImGui::TextUnformatted("SÓLIDO PERSONALIZADO");
    ImGui::PopStyleColor();

    ImGui::SameLine();

    const float closeButtonSize = 24.0f;
    const float closeButtonX =
        ImGui::GetCursorPosX() +
        ImGui::GetContentRegionAvail().x -
        closeButtonSize;

    ImGui::SetCursorPosX(closeButtonX);

    if (ImGui::Button("X", ImVec2(closeButtonSize, closeButtonSize)))
    {
        m_context->showCustomSolidPanel = false;
    }
}

void CustomSolidPanel::drawPreviews()
{
    DrawSectionTitle("PREVIEW");

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;

    const float previewWidth = (availableWidth - spacing) * 0.5f;
    const float previewHeight = previewWidth * 0.78f;

    m_topPreview.draw(m_state, ImVec2(previewWidth, previewHeight));

    ImGui::SameLine();

    m_sidePreview.draw(m_state, ImVec2(previewWidth, previewHeight));
}

void CustomSolidPanel::drawFields()
{
    DrawSectionTitle("CONFIGURAÇÕES");

    DrawFieldLabel("Nome");
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##CustomSolidName", m_state.name, sizeof(m_state.name));

    DrawFieldLabel("Lados");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputInt("##CustomSolidSides", &m_state.sides))
    {
        clampState();
    }

    DrawFieldLabel("Raio inferior");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::DragFloat("##CustomSolidBottomRadius", &m_state.bottomRadius, 0.1f, 0.0f, 100.0f, "%.2f"))
    {
        clampState();
    }

    DrawFieldLabel("Raio superior");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::DragFloat("##CustomSolidTopRadius", &m_state.topRadius, 0.1f, 0.0f, 100.0f, "%.2f"))
    {
        clampState();
    }

    DrawFieldLabel("Altura");
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::DragFloat("##CustomSolidHeight", &m_state.height, 0.1f, 0.1f, 100.0f, "%.2f"))
    {
        clampState();
    }
}

void CustomSolidPanel::drawActions()
{
    if (ImGui::Button("Adicionar", ImVec2(-1.0f, 40.0f)))
    {
        submit();
    }
}

void CustomSolidPanel::clampState()
{
    m_state.sides = std::clamp(m_state.sides, 3, 64);
    m_state.bottomRadius = std::max(0.0f, m_state.bottomRadius);
    m_state.topRadius = std::max(0.0f, m_state.topRadius);
    m_state.height = std::max(0.1f, m_state.height);
}

void CustomSolidPanel::submit()
{
    clampState();

    std::strncpy(m_context->customSolidName, m_state.name, sizeof(m_context->customSolidName) - 1);
    m_context->customSolidName[sizeof(m_context->customSolidName) - 1] = '\0';

    m_context->customSolidSides = m_state.sides;
    m_context->customSolidBottomRadius = m_state.bottomRadius;
    m_context->customSolidTopRadius = m_state.topRadius;
    m_context->customSolidHeight = m_state.height;

    m_eventBus->emit(EventType::AddCustomSolid, m_state.sides);

    m_context->showCustomSolidPanel = false;
}