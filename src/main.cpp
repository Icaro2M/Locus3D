#include "core/WindowManager.h"
#include "graphics/Shader.h"
#include "graphics/Renderer.h"
#include "geometry/Mesh.h"

int main()
{
    WindowManager window(800, 600, "Locus3D");

    float vertices[] =
    {
        -0.5f,-0.5f,0.0f,
         0.5f,-0.5f,0.0f,
         0.0f, 0.5f,0.0f
    };

    Mesh triangle(vertices, sizeof(vertices));

    Shader shader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\fragment.glsl"
    );

    Renderer renderer;

    while (!window.shouldClose())
    {
        renderer.clear();
        renderer.draw(triangle, shader);

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}