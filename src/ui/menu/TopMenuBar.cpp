#include "TopMenuBar.h"

#include "WindowControlButton.h"
#include "../layout/UILayoutConstants.h"

#include <imgui.h>

namespace
{
    constexpr float WindowControlsWidth = ui::menu::WindowControlButtonWidth * 3.0f;
    constexpr float InitialDragStartX = 150.0f;
}

TopMenuBar::TopMenuBar(AppEventBus* eventBus, UIContext* context, WindowController* windowController)
    : m_eventBus(eventBus),
    m_context(context),
    m_windowController(windowController),
    m_fileMenu(eventBus)
{
}

void TopMenuBar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (m_windowController != nullptr)
    {
        m_windowController->setTitleBarMetrics(
            ui::layout::TitleBarHeight,
            InitialDragStartX,
            WindowControlsWidth
        );
    }

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, ui::layout::TitleBarHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_MenuBar;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 7.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.060f, 0.064f, 0.076f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.060f, 0.064f, 0.076f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.14f, 0.16f, 0.20f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.22f, 0.30f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.12f, 0.30f, 0.60f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.88f, 0.90f, 0.94f, 1.0f));

    if (ImGui::Begin("TopMenuBar", nullptr, flags))
    {
        if (ImGui::BeginMenuBar())
        {
            ImGui::SetCursorPosX(12.0f);
            m_fileMenu.draw();

            const float dragStartX = ImGui::GetCursorPosX() + 20.0f;

            if (m_windowController != nullptr)
            {
                m_windowController->setTitleBarMetrics(
                    ui::layout::TitleBarHeight,
                    dragStartX,
                    WindowControlsWidth
                );
            }

            const char* title = "Locus3D";
            const float titleWidth = ImGui::CalcTextSize(title).x;
            const float centeredTitleX = (viewport->Size.x - titleWidth) * 0.5f;
            const float minTitleX = dragStartX;

            ImGui::SetCursorPosX(centeredTitleX > minTitleX ? centeredTitleX : minTitleX);
            ImGui::TextDisabled("%s", title);

            ImGui::SetCursorPosX(viewport->Size.x - WindowControlsWidth);

            if (ui::menu::WindowControlButton(
                "##window_minimize",
                ui::menu::WindowControlIcon::Minimize,
                "Minimizar"
            ))
            {
                if (m_windowController != nullptr)
                {
                    m_windowController->minimize();
                }
            }

            ImGui::SameLine(0.0f, 0.0f);

            const bool maximized =
                m_windowController != nullptr && m_windowController->isMaximized();

            if (ui::menu::WindowControlButton(
                "##window_maximize",
                maximized
                ? ui::menu::WindowControlIcon::Restore
                : ui::menu::WindowControlIcon::Maximize,
                maximized ? "Restaurar" : "Maximizar"
            ))
            {
                if (m_windowController != nullptr)
                {
                    m_windowController->toggleMaximize();
                }
            }

            ImGui::SameLine(0.0f, 0.0f);

            if (ui::menu::WindowControlButton(
                "##window_close",
                ui::menu::WindowControlIcon::Close,
                "Fechar",
                true
            ))
            {
                if (m_windowController != nullptr)
                {
                    m_windowController->close();
                }
            }

            ImGui::EndMenuBar();
        }
    }

    ImGui::End();

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(4);
}
