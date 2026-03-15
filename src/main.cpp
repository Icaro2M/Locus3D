#include "core/WindowManager.h"
#include "graphics/Renderer.h"
#include "graphics/Shader.h"
#include "scene/Scene.h"
#include "scene/SceneObject.h"
#include "scene/Camera.h"
#include "geometry/primitives/PrimitiveFactory.h"

int main()
{
    WindowManager window(800, 600, "Locus3D");

    glEnable(GL_DEPTH_TEST);

    Mesh cube = PrimitiveFactory::createCube();
    Mesh box = PrimitiveFactory::createBox(1.5f, 1.0f, 0.5f);
    Mesh tetra = PrimitiveFactory::createTetrahedron();

    SceneObject cubeObject(cube);
    SceneObject boxObject(box);
    SceneObject tetraObject(tetra);

    cubeObject.getTransform().setPosition(glm::vec3(-2.0f, 0.0f, 0.0f));
    boxObject.getTransform().setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    tetraObject.getTransform().setPosition(glm::vec3(2.0f, 0.0f, 0.0f));

    Scene scene;
    scene.addObject(cubeObject);
    scene.addObject(boxObject);
    scene.addObject(tetraObject);

    Shader shader(
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\vertex.glsl",
        "C:\\Users\\icaro\\Projetos\\TCC\\Locus3D\\assets\\shaders\\basic\\fragment.glsl"
    );

    Renderer renderer;

    Camera camera;
    camera.setPosition(glm::vec3(0.0f, 0.0f, 6.0f));
    camera.setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera.setUp(glm::vec3(0.0f, 1.0f, 0.0f));
    camera.setAspectRatio(800.0f / 600.0f);

    float angle = 0.0f;

    float cameraSpeed = 1.0f;
    float zoomSpeed = 0.05f;

    while (!window.shouldClose())
    {
        renderer.clear();

        if (glfwGetKey(window.getWindow(), GLFW_KEY_A) == GLFW_PRESS)
        {
            camera.setYaw(camera.getYaw() - cameraSpeed);
        }

        if (glfwGetKey(window.getWindow(), GLFW_KEY_D) == GLFW_PRESS)
        {
            camera.setYaw(camera.getYaw() + cameraSpeed);
        }

        if (glfwGetKey(window.getWindow(), GLFW_KEY_W) == GLFW_PRESS)
        {
            camera.setPitch(camera.getPitch() + cameraSpeed);
        }

        if (glfwGetKey(window.getWindow(), GLFW_KEY_S) == GLFW_PRESS)
        {
            camera.setPitch(camera.getPitch() - cameraSpeed);
        }

        if (glfwGetKey(window.getWindow(), GLFW_KEY_Q) == GLFW_PRESS)
        {
            camera.setDistance(camera.getDistance() + zoomSpeed);
        }

        if (glfwGetKey(window.getWindow(), GLFW_KEY_E) == GLFW_PRESS)
        {
            camera.setDistance(camera.getDistance() - zoomSpeed);
        }

        camera.updateOrbitPosition();

        renderer.renderScene(scene, camera, shader);

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}