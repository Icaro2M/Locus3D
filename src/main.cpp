#include "core/WindowManager.h"
#include "graphics/Shader.h"
#include "graphics/VertexArray.h"
#include "graphics/VertexBuffer.h"

int main()
{
    WindowManager window(800, 600, "Locus3D");

    float vertices[] = {
        -0.5f,-0.5f,0.0f,
         0.5f,-0.5f,0.0f,
         0.0f, 0.5f,0.0f
    };

    VertexArray vao;
    vao.bind();

    VertexBuffer vbo(vertices, sizeof(vertices));

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        3 * sizeof(float),
        (void*)0
    );

    glEnableVertexAttribArray(0);

    Shader shader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\fragment.glsl"
    );

    while (!window.shouldClose())
    {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.use();
        vao.bind();

        glDrawArrays(GL_TRIANGLES, 0, 3);

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}