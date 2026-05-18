#include "UIFonts.h"

namespace
{
    constexpr float TopBarCompactFontSize = 18.0f;
    constexpr float TopBarComfortableFontSize = 20.0f;

    ImFont* g_defaultFont = nullptr;
    ImFont* g_topBarCompactFont = nullptr;
    ImFont* g_topBarComfortableFont = nullptr;

    bool UseCompactTopBar(float viewportHeight)
    {
        return viewportHeight >= 1000.0f;
    }
}

namespace ui::fonts
{
    void LoadFonts(ImGuiIO& io)
    {
        g_defaultFont = io.Fonts->AddFontFromFileTTF(
            "assets/fonts/Roboto-Regular.ttf",
            16.0f
        );

        g_topBarCompactFont = io.Fonts->AddFontFromFileTTF(
            "assets/fonts/Roboto-Regular.ttf",
            TopBarCompactFontSize
        );

        g_topBarComfortableFont = io.Fonts->AddFontFromFileTTF(
            "assets/fonts/Roboto-Regular.ttf",
            TopBarComfortableFontSize
        );

        if (g_defaultFont != nullptr)
        {
            io.FontDefault = g_defaultFont;
        }
    }

    ImFont* Default()
    {
        return g_defaultFont;
    }

    ImFont* TopBar(float viewportHeight)
    {
        ImFont* preferredFont = UseCompactTopBar(viewportHeight)
            ? g_topBarCompactFont
            : g_topBarComfortableFont;

        return preferredFont != nullptr ? preferredFont : g_defaultFont;
    }

    float TopBarSize(float viewportHeight)
    {
        return UseCompactTopBar(viewportHeight)
            ? TopBarCompactFontSize
            : TopBarComfortableFontSize;
    }
}
