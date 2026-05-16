#include "FileMenu.h"

#include <imgui.h>

#include "../TopMenuTypes.h"

FileMenu::FileMenu(AppEventBus* eventBus)
    : m_eventBus(eventBus)
{
}

void FileMenu::draw()
{
    if (!ImGui::BeginMenu(ui::menu::FileMenuLabel))
    {
        return;
    }

    if (ImGui::MenuItem(ui::menu::OpenSceneLabel, ui::menu::OpenSceneShortcut))
    {
        m_eventBus->emit(EventType::FileOpen);
    }

    ImGui::Separator();

    if (ImGui::MenuItem(ui::menu::SaveSceneLabel, ui::menu::SaveSceneShortcut))
    {
        m_eventBus->emit(EventType::FileSave);
    }

    if (ImGui::MenuItem(ui::menu::SaveSceneAsLabel, ui::menu::SaveSceneAsShortcut))
    {
        m_eventBus->emit(EventType::FileSaveAs);
    }

    ImGui::EndMenu();
}