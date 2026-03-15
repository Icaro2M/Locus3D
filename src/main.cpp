#include "core/WindowManager.h"
#include "graphics/Shader.h"
#include "graphics/Renderer.h"
#include "geometry/Mesh.h"
#include "scene/SceneObject.h"
#include "scene/Scene.h"
#include "scene/Camera.h"

int main()
{
    WindowManager window(800, 600, "Locus3D");

    glEnable(GL_DEPTH_TEST);

    float cubeVertices[] =
    {
        // Face traseira
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,

        // Face frontal
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,

        // Face esquerda
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,

        // Face direita
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,

         // Face inferior
         -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
         -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
          0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
          0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,

          // Face superior
          -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
          -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
           0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
           0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
    };

    unsigned int cubeIndices[] =
    {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23
    };

    float prismVertices[] =
    {
        // Face 1
         0.0f,  0.6f,  0.0f,   0.0f,  0.4472f,  0.8944f,
        -0.5f, -0.3f,  0.5f,   0.0f,  0.4472f,  0.8944f,
         0.5f, -0.3f,  0.5f,   0.0f,  0.4472f,  0.8944f,

         // Face 2
          0.0f,  0.6f,  0.0f,   0.8944f,  0.4472f,  0.0f,
          0.5f, -0.3f,  0.5f,   0.8944f,  0.4472f,  0.0f,
          0.5f, -0.3f, -0.5f,   0.8944f,  0.4472f,  0.0f,

          // Face 3
           0.0f,  0.6f,  0.0f,   0.0f,  0.4472f, -0.8944f,
           0.5f, -0.3f, -0.5f,   0.0f,  0.4472f, -0.8944f,
          -0.5f, -0.3f, -0.5f,   0.0f,  0.4472f, -0.8944f,

          // Face 4
           0.0f,  0.6f,  0.0f,  -0.8944f,  0.4472f,  0.0f,
          -0.5f, -0.3f, -0.5f,  -0.8944f,  0.4472f,  0.0f,
          -0.5f, -0.3f,  0.5f,  -0.8944f,  0.4472f,  0.0f
    };

    unsigned int prismIndices[] =
    {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,
        9, 10, 11
    };

    Mesh cube(cubeVertices, sizeof(cubeVertices), cubeIndices, 36);
    Mesh prism(prismVertices, sizeof(prismVertices), prismIndices, 36);

    SceneObject cubeObject(cube);
    SceneObject prismObject(prism);

    cubeObject.getTransform().setPosition(glm::vec3(-1.1f, 0.0f, 0.0f));
    prismObject.getTransform().setPosition(glm::vec3(1.1f, 0.0f, 0.0f));

    Scene scene;
    scene.addObject(cubeObject);
    scene.addObject(prismObject);

    Shader shader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\fragment.glsl"
    );

    Renderer renderer;

    Camera camera;
    camera.setPosition(glm::vec3(0.0f, 0.0f, 4.0f));
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

        cubeObject.getTransform().setRotation(glm::vec3(angle, angle, 0.0f));
        prismObject.getTransform().setRotation(glm::vec3(0.0f, angle, angle));

        shader.use();
        shader.setMat4("u_View", camera.getViewMatrix());
        shader.setMat4("u_Projection", camera.getProjectionMatrix());

        for (SceneObject* object : scene.getObjects())
        {
            shader.setMat4("u_Model", object->getTransform().getModelMatrix());
            renderer.draw(object->getMesh(), shader);
        }

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}