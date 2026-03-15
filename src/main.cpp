#include "core/WindowManager.h"
#include "graphics/Shader.h"
#include "graphics/Renderer.h"
#include "geometry/Mesh.h"
#include "math/Transform.h"
#include "scene/Camera.h"

#include <glad/glad.h>
#include <glm/glm/glm.hpp>

int main()
{
    WindowManager window(800, 600, "Locus3D");

    glEnable(GL_DEPTH_TEST);

    float vertices[] =
    {
  
        -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,


        -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,


        -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,

    
         0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,

 
         -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,
         -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,
          0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,
          0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,

         
          -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,
          -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
           0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,
           0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f
    };

    unsigned int indices[] =
    {
        0, 1, 2,   0, 2, 3,    
        4, 5, 6,   4, 6, 7,     
        8, 9, 10,  8, 10, 11,    
        12, 13, 14, 12, 14, 15,   
        16, 17, 18, 16, 18, 19,  
        20, 21, 22, 20, 22, 23   
    };

    Mesh cube(vertices, sizeof(vertices), indices, 36);


    Shader shader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\fragment.glsl"
    );

    Renderer renderer;

    Transform transform;
    transform.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    transform.setScale(glm::vec3(1.0f, 1.0f, 1.0f));

    Camera camera;
    camera.setPosition(glm::vec3(0.0f, 0.0f, 3.0f));
    camera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.setUp(glm::vec3(0.0f, 1.0f, 0.0f));
    camera.setAspectRatio(800.0f / 600.0f);

    float angle = 0.0f;

    while (!window.shouldClose())
    {
        renderer.clear();

        angle += 0.3f;
        if (angle >= 360.0f)
            angle -= 360.0f;

        transform.setRotation(glm::vec3(angle, angle, 0.0f));

        shader.use();
        shader.setMat4("u_Model", transform.getModelMatrix());
        shader.setMat4("u_View", camera.getViewMatrix());
        shader.setMat4("u_Projection", camera.getProjectionMatrix());

        renderer.draw(cube, shader);

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}
