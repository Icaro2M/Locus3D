#include "graphics/window/Window.h"

#include <iostream>

int main()
{
    locus::graphics::Window window;

    locus::graphics::WindowCreateInfo info;
    info.width = 1280;
    info.height = 720;
    info.title = "Locus3D";

    auto result = window.create(info);

    if (!result)
    {
        std::cerr << result.error().message << '\n';
        return 1;
    }

    while (!window.should_close())
    {
        window.poll_events();
        window.swap_buffers();
    }

    return 0;
}