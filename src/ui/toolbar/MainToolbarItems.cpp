#include "MainToolbarItems.h"

#include "../../resources/AssetPaths.h"

namespace ui::toolbar
{
    const MainToolbarItem* GetMainToolbarItems(int& count)
    {
        static const MainToolbarItem items[] = {
            {
                MainToolbarItemType::Button,
                MainToolbarAction::Select,
                "main_toolbar_select",
                "Selecionar / modo objeto (F)",
                AssetPaths::toolbarIcon("select.png")
            },
            {
                MainToolbarItemType::Button,
                MainToolbarAction::ExtrudeFace,
                "main_toolbar_extrude_face",
                "Extrusão de face (T)",
                AssetPaths::toolbarIcon("extrude-face.png")
            },
            {
                MainToolbarItemType::Button,
                MainToolbarAction::MoveFace,
                "main_toolbar_move_face",
                "Mover face (M)",
                AssetPaths::toolbarIcon("move-face.png")
            },
            {
                MainToolbarItemType::Button,
                MainToolbarAction::ScaleFace,
                "main_toolbar_scale_face",
                "Escalar face (S)",
                AssetPaths::toolbarIcon("scale-face.png")
            },
            {
                MainToolbarItemType::Separator,
                MainToolbarAction::None,
                nullptr,
                nullptr,
                ""
            },
            {
                MainToolbarItemType::Dropdown,
                MainToolbarAction::PrimitiveDropdown,
                "main_toolbar_primitives",
                "Adicionar sólido",
                AssetPaths::primitiveIcon("cube.png")
            },
            {
                MainToolbarItemType::Separator,
                MainToolbarAction::None,
                nullptr,
                nullptr,
                ""
            },
            {
                MainToolbarItemType::Button,
                MainToolbarAction::CustomSolid,
                "main_toolbar_custom_solid",
                "Sólido personalizado",
                AssetPaths::primitiveIcon("custom-solid.png")
            }
        };

        count = static_cast<int>(sizeof(items) / sizeof(items[0]));
        return items;
    }
}