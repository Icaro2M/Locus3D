#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "core/WindowManager.h"
#include "graphics/Renderer.h"
#include "graphics/Shader.h"
#include "scene/Scene.h"
#include "scene/SceneObject.h"
#include "scene/Camera.h"
#include "scene/CameraController.h"
#include "geometry/primitives/PrimitiveFactory.h"
#include "tools/AxisRenderer.h"
#include "tools/GridRenderer.h"
#include "tools/selection/ObjectSelector.h"
#include "tools/selection/SelectionOutlineRenderer.h"

Camera* g_Camera = nullptr;
CameraController* g_CameraController = nullptr;

void scrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
    if (g_Camera && g_CameraController)
    {
        g_CameraController->processScroll(*g_Camera, static_cast<float>(yOffset));
    }
}

int main()
{
    WindowManager window(800, 600, "Locus3D");

    glEnable(GL_DEPTH_TEST);

    AxisRenderer axisRenderer;
    GridRenderer gridRenderer;
    ObjectSelector selector;
    SelectionOutlineRenderer selectionOutlineRenderer;

    Mesh cube = PrimitiveFactory::createCube();
    Mesh box = PrimitiveFactory::createBox(1.5f, 1.0f, 0.5f);
    Mesh tetra = PrimitiveFactory::createTetrahedron();

    SceneObject cubeObject(cube);
    SceneObject boxObject(box);
    SceneObject tetraObject(tetra);

    cubeObject.getTransform().setPosition(glm::vec3(-1.5f, 0.0f, 0.0f));
    boxObject.getTransform().setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    tetraObject.getTransform().setPosition(glm::vec3(1.5f, 0.0f, 0.0f));

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
    CameraController cameraController;

    g_Camera = &camera;
    g_CameraController = &cameraController;

    glfwSetScrollCallback(window.getWindow(), scrollCallback);

    float angle = 0.0f;

    SceneObject* selectedObject = nullptr;
    bool leftMouseWasPressed = false;

    while (!window.shouldClose())
    {
        renderer.clear();

        cameraController.processMouse(window.getWindow(), camera);

        angle += 0.3f;
        if (angle >= 360.0f)
            angle -= 360.0f;

        cubeObject.getTransform().setRotation(glm::vec3(angle, angle, 0.0f));
        boxObject.getTransform().setRotation(glm::vec3(0.0f, angle, angle));
        tetraObject.getTransform().setRotation(glm::vec3(angle, 0.0f, angle));

        bool leftMousePressed =
            glfwGetMouseButton(window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (leftMousePressed && !leftMouseWasPressed)
        {
            selectedObject = selector.selectObject(window.getWindow(), camera, scene);
        }

        leftMouseWasPressed = leftMousePressed;

        renderer.renderScene(scene, camera, shader);
        gridRenderer.render(camera);
        axisRenderer.render(camera);

        if (selectedObject != nullptr)
        {
            selectionOutlineRenderer.render(*selectedObject, camera);
        }

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}