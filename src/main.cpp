#include "core/WindowManager.h"
#include "graphics/Shader.h"
#include "graphics/Renderer.h"
#include "geometry/Mesh.h"
#include "math/Transform.h"

#include <glm/glm/glm.hpp>

int main()
{
    WindowManager window(800, 600, "Locus3D");

    float vertices[] =
    {
        -0.5f, -0.5f, -0.5f, 
         0.5f, -0.5f, -0.5f, 
         0.5f,  0.5f, -0.5f, 
        -0.5f,  0.5f, -0.5f, 

        -0.5f, -0.5f,  0.5f, 
         0.5f, -0.5f,  0.5f, 
         0.5f,  0.5f,  0.5f, 
        -0.5f,  0.5f,  0.5f  
    };

    unsigned int indices[] =
    {

        0, 1, 2,
        0, 2, 3,


        4, 5, 6,
        4, 6, 7,


        0, 3, 7,
        0, 7, 4,


        1, 5, 6,
        1, 6, 2,


        0, 4, 5,
        0, 5, 1,


        3, 2, 6,
        3, 6, 7
    };

    Mesh cube(vertices, sizeof(vertices), indices, 36);

    Shader shader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\fragment.glsl"
    );

    Renderer renderer;

    Transform transform;
    transform.setPosition(glm::vec3(0.3f, 0.0f, 0.0f));
    transform.setScale(glm::vec3(0.7f, 0.7f, 1.0f));



    while (!window.shouldClose())
    {   


        shader.use();
        shader.setMat4("u_Model", transform.getModelMatrix());

        renderer.draw(cube, shader);

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}