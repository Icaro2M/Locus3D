#include "core/WindowManager.h"

int main()
{
    WindowManager window(800, 600, "Locus3D");

    while (!window.shouldClose())
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}