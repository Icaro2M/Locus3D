#include "MainToolbar.h"

#include <imgui.h>

#include "../../resources/AssetPaths.h"
#include "../widgets/buttons/DropdownButton.h"
#include "../widgets/buttons/PopupMenuItemButton.h"
#include "../widgets/buttons/ToggleIconButton.h"
#include "../layout/UILayoutConstants.h"

namespace
{
    constexpr float ToolbarButtonSize = ui::layout::MainToolbarHeight * 0.6f;
    constexpr float SeparatorWidth = ui::layout::MainToolbarHeight * 0.3f;
    constexpr float SeparatorHeightRatio = 0.6f;

    ui::ButtonVisualStyle MainToolbarButtonStyle()
    {
        ui::ButtonVisualStyle style;

        style.size = ImVec2(ToolbarButtonSize, ToolbarButtonSize);
        style.rounding = 10.0f;
        style.borderThickness = 1.0f;
        style.iconScale = 0.65f;

        style.backgroundColor = ImVec4(0.10f, 0.11f, 0.13f, 1.0f);
        style.hoverColor = ImVec4(0.15f, 0.17f, 0.20f, 1.0f);

        style.activeColor = ImVec4(0.05f, 0.36f, 0.78f, 1.0f);
        style.activeHoverColor = ImVec4(0.07f, 0.43f, 0.92f, 1.0f);

        style.borderColor = ImVec4(0.24f, 0.26f, 0.30f, 1.0f);
        style.activeBorderColor = ImVec4(0.25f, 0.55f, 1.0f, 1.0f);

        style.iconColor = ImVec4(0.72f, 0.75f, 0.80f, 1.0f);
        style.activeIconColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        style.disabledBackgroundColor = ImVec4(0.08f, 0.09f, 0.10f, 1.0f);
        style.disabledBorderColor = ImVec4(0.16f, 0.17f, 0.19f, 1.0f);
        style.disabledIconColor = ImVec4(0.38f, 0.40f, 0.44f, 1.0f);

        return style;
    }
}

MainToolbar::MainToolbar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus),
    m_context(context)
{
}

void MainToolbar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x, viewport->WorkPos.y));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x, ui::layout::MainToolbarHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.085f, 0.090f, 0.105f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(16.0f, 0.0f));

    if (ImGui::Begin("MainToolbar", nullptr, flags))
    {
        float buttonOffsetY = (ImGui::GetWindowHeight() - ToolbarButtonSize) * 0.5f;
        if (buttonOffsetY < 0.0f)
        {
            buttonOffsetY = 0.0f;
        }

        ImGui::SetCursorPosY(buttonOffsetY);

        int itemCount = 0;
        const ui::toolbar::MainToolbarItem* items = ui::toolbar::GetMainToolbarItems(itemCount);

        for (int i = 0; i < itemCount; ++i)
        {
            if (i > 0)
            {
                ImGui::SameLine();
            }

            drawToolbarItem(items[i]);
        }

        drawToolbarBottomBorder();
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void MainToolbar::drawToolbarItem(const ui::toolbar::MainToolbarItem& item)
{
    using namespace ui::toolbar;

    switch (item.type)
    {
    case MainToolbarItemType::Button:
    {
        bool active = isActionActive(item.action);

        if (ui::ToggleIconButton({
            item.id,
            item.tooltip,
            active,
            true,
            item.iconPath,
            MainToolbarButtonStyle()
            }))
        {
            handleAction(item.action);
        }

        break;
    }

    case MainToolbarItemType::Dropdown:
    {
        drawPrimitiveDropdown(item);
        break;
    }

    case MainToolbarItemType::Separator:
    {
        drawSeparator();
        break;
    }
    }
}

void MainToolbar::drawPrimitiveDropdown(const ui::toolbar::MainToolbarItem& item)
{
    bool popupOpen = ImGui::IsPopupOpen("MainToolbarPrimitivePopup");

    bool pressed = ui::DropdownButton({
        item.id,
        item.tooltip,
        popupOpen,
        true,
        item.iconPath,
        MainToolbarButtonStyle()
        });

    ImVec2 buttonMin = ImGui::GetItemRectMin();
    ImVec2 buttonMax = ImGui::GetItemRectMax();

    if (pressed)
    {
        ImGui::OpenPopup("MainToolbarPrimitivePopup");
    }

    ImVec2 popupPos = ImVec2(buttonMin.x, buttonMax.y + 8.0f);

    ImGui::SetNextWindowPos(popupPos, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.065f, 0.070f, 0.082f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.20f, 0.24f, 1.0f));

    if (ImGui::BeginPopup("MainToolbarPrimitivePopup"))
    {
        ui::PopupMenuItemStyle itemStyle;
        itemStyle.rounding = 0.0f;

        if (ui::PopupMenuItemButton({ "primitive_cube", "Cube", false, true, AssetPaths::primitiveIcon("cube.png"), itemStyle }))
        {
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::AddPrimitive, 0);
            ImGui::CloseCurrentPopup();
        }

        if (ui::PopupMenuItemButton({ "primitive_sphere", "Sphere", false, true, AssetPaths::primitiveIcon("sphere.png"), itemStyle }))
        {
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::AddPrimitive, 1);
            ImGui::CloseCurrentPopup();
        }

        if (ui::PopupMenuItemButton({ "primitive_cone", "Cone", false, true, AssetPaths::primitiveIcon("cone.png"), itemStyle }))
        {
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::AddPrimitive, 2);
            ImGui::CloseCurrentPopup();
        }

        if (ui::PopupMenuItemButton({ "primitive_cylinder", "Cylinder", false, true, AssetPaths::primitiveIcon("cylinder.png"), itemStyle }))
        {
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::AddPrimitive, 3);
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

void MainToolbar::drawToolbarBottomBorder()
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    drawList->AddLine(
        ImVec2(pos.x, pos.y + size.y - 1.0f),
        ImVec2(pos.x + size.x, pos.y + size.y - 1.0f),
        IM_COL32(42, 46, 54, 255),
        1.0f
    );
}

void MainToolbar::drawSeparator()
{
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImVec2 windowPos = ImGui::GetWindowPos();

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float toolbarHeight = ImGui::GetWindowHeight();
    float separatorHeight = ToolbarButtonSize * SeparatorHeightRatio;

    float x = cursor.x + SeparatorWidth * 0.5f;
    float y = windowPos.y + (toolbarHeight - separatorHeight) * 0.5f;

    drawList->AddLine(
        ImVec2(x, y),
        ImVec2(x, y + separatorHeight),
        IM_COL32(50, 54, 64, 255),
        1.0f
    );

    ImGui::Dummy(ImVec2(SeparatorWidth, ToolbarButtonSize));
}

bool MainToolbar::isActionActive(ui::toolbar::MainToolbarAction action) const
{
    using namespace ui::toolbar;

    switch (action)
    {
    case MainToolbarAction::Select:
        return m_context->activeToolId == 0;

    case MainToolbarAction::ExtrudeFace:
        return m_context->activeToolId == 1;

    case MainToolbarAction::MoveFace:
        return m_context->activeToolId == 2;

    case MainToolbarAction::ScaleFace:
        return m_context->activeToolId == 3;

    case MainToolbarAction::CustomSolid:
        return m_context->showCustomSolidPanel;

    default:
        return false;
    }
}

void MainToolbar::handleAction(ui::toolbar::MainToolbarAction action)
{
    using namespace ui::toolbar;

    switch (action)
    {
    case MainToolbarAction::Select:
        m_context->activeToolId = 0;
        m_context->showCustomSolidPanel = false;
        m_eventBus->emit(EventType::InputKeyF);
        break;

    case MainToolbarAction::ExtrudeFace:
        m_context->activeToolId = 1;
        m_context->showCustomSolidPanel = false;
        m_eventBus->emit(EventType::InputKeyT);
        break;

    case MainToolbarAction::MoveFace:
        m_context->activeToolId = 2;
        m_context->showCustomSolidPanel = false;
        m_eventBus->emit(EventType::InputKeyM);
        break;

    case MainToolbarAction::ScaleFace:
        m_context->activeToolId = 3;
        m_context->showCustomSolidPanel = false;
        m_eventBus->emit(EventType::InputKeyS);
        break;

    case MainToolbarAction::CustomSolid:
        m_context->showCustomSolidPanel = !m_context->showCustomSolidPanel;
        break;

    default:
        break;
    }
}