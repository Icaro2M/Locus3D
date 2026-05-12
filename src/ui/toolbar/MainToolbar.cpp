#include "MainToolbar.h"

#include <imgui.h>

#include "../../resources/AssetPaths.h"
#include "../widgets/buttons/DropdownButton.h"
#include "../widgets/buttons/PopupMenuItemButton.h"
#include "../widgets/buttons/ToggleIconButton.h"

MainToolbar::MainToolbar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus),
    m_context(context)
{
}

void MainToolbar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 66.0f));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.085f, 0.090f, 0.105f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 0.0f));

    if (ImGui::Begin("MainToolbar", nullptr, flags))
    {
        drawSelectionGroup();

        ImGui::SameLine();
        drawSeparator();

        ImGui::SameLine();
        drawPrimitiveGroup();

        ImGui::SameLine();
        drawSeparator();

        ImGui::SameLine();
        drawFaceToolGroup();

        ImGui::SameLine();
        drawSeparator();

        ImGui::SameLine();
        drawCustomSolidGroup();
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
}

void MainToolbar::drawSelectionGroup()
{
    bool isSelectionActive = m_context->activeToolId == 0;

    if (ui::ToggleIconButton({
        "main_toolbar_select",
        "Selecionar face (F)",
        isSelectionActive,
        true,
        AssetPaths::toolbarIcon("select.png")
        }))
    {
        m_context->activeToolId = 0;
        m_context->showCustomSolidPanel = false;
        m_eventBus->emit(EventType::InputKeyF);
    }
}

void MainToolbar::drawPrimitiveGroup()
{
    bool popupOpen = ImGui::IsPopupOpen("MainToolbarPrimitivePopup");

    if (ui::DropdownButton({
        "main_toolbar_primitives",
        "Adicionar sólido",
        popupOpen,
        true,
        AssetPaths::primitiveIcon("cube.png")
        }))
    {
        ImGui::OpenPopup("MainToolbarPrimitivePopup");
    }

    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 2.0f));

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.065f, 0.070f, 0.082f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.18f, 0.20f, 0.24f, 1.0f));

    if (ImGui::BeginPopup("MainToolbarPrimitivePopup"))
    {
        if (ui::PopupMenuItemButton({
            "primitive_cube",
            "Cube",
            false,
            true,
            AssetPaths::primitiveIcon("cube.png")
            }))
        {
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::AddPrimitive, 0);
            ImGui::CloseCurrentPopup();
        }

        if (ui::PopupMenuItemButton({
            "primitive_sphere",
            "Sphere",
            false,
            true,
            AssetPaths::primitiveIcon("sphere.png")
            }))
        {
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::AddPrimitive, 1);
            ImGui::CloseCurrentPopup();
        }

        if (ui::PopupMenuItemButton({
            "primitive_cone",
            "Cone",
            false,
            true,
            AssetPaths::primitiveIcon("cone.png")
            }))
        {
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::AddPrimitive, 2);
            ImGui::CloseCurrentPopup();
        }

        if (ui::PopupMenuItemButton({
            "primitive_cylinder",
            "Cylinder",
            false,
            true,
            AssetPaths::primitiveIcon("cylinder.png")
            }))
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

void MainToolbar::drawFaceToolGroup()
{
    bool isExtrudeActive = m_context->activeToolId == 1;

    if (ui::ToggleIconButton({
        "main_toolbar_extrude_face",
        "Extrusão de face (T)",
        isExtrudeActive,
        true,
        AssetPaths::toolbarIcon("extrude-face.png")
        }))
    {
        m_context->activeToolId = 1;
        m_context->showCustomSolidPanel = false;
        m_eventBus->emit(EventType::InputKeyT);
    }

    ImGui::SameLine();

    bool isMoveFaceActive = m_context->activeToolId == 2;

    if (ui::ToggleIconButton({
        "main_toolbar_move_face",
        "Mover face (M)",
        isMoveFaceActive,
        true,
        AssetPaths::toolbarIcon("move-face.png")
        }))
    {
        m_context->activeToolId = 2;
        m_context->showCustomSolidPanel = false;
        m_eventBus->emit(EventType::InputKeyM);
    }

    ImGui::SameLine();

    bool isScaleFaceActive = m_context->activeToolId == 3;

    if (ui::ToggleIconButton({
        "main_toolbar_scale_face",
        "Escalar face (S)",
        isScaleFaceActive,
        true,
        AssetPaths::toolbarIcon("scale-face.png")
        }))
    {
        m_context->activeToolId = 3;
        m_context->showCustomSolidPanel = false;
        m_eventBus->emit(EventType::InputKeyS);
    }
}

void MainToolbar::drawCustomSolidGroup()
{
    bool isCustomSolidActive = m_context->showCustomSolidPanel;

    if (ui::ToggleIconButton({
        "main_toolbar_custom_solid",
        "Sólido personalizado",
        isCustomSolidActive,
        true,
        AssetPaths::primitiveIcon("custom-solid.png")
        }))
    {
        m_context->showCustomSolidPanel = !m_context->showCustomSolidPanel;
    }
}

void MainToolbar::drawSeparator()
{
    ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float height = 34.0f;
    float x = cursor.x + 4.0f;
    float y = cursor.y + 2.0f;

    drawList->AddLine(
        ImVec2(x, y),
        ImVec2(x, y + height),
        IM_COL32(58, 62, 70, 255),
        1.0f
    );

    ImGui::Dummy(ImVec2(12.0f, 38.0f));
}