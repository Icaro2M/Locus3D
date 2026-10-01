#include "graphics/viewport/Viewport.h"

namespace locus::graphics {
void Viewport::sync_with_window(const Window& window)
{
    set_rect(ViewportRect{0, 0, window.framebuffer_width(), window.framebuffer_height()});
}
}
