#pragma once

#include <string>

namespace ui::toolbar
{
    enum class MainToolbarItemType
    {
        Button,
        Dropdown,
        Separator
    };

    enum class MainToolbarAction
    {
        None,
        Select,
        ExtrudeFace,
        MoveFace,
        ScaleFace,
        PrimitiveDropdown,
        CustomSolid
    };

    struct MainToolbarItem
    {
        MainToolbarItemType type;
        MainToolbarAction action;
        const char* id;
        const char* tooltip;
        std::string iconPath;
    };

    const MainToolbarItem* GetMainToolbarItems(int& count);
}