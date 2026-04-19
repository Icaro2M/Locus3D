#include <iostream>

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
#include "tools/selection/FaceSelector.h"
#include "tools/selection/FaceSelection.h"
#include "tools/selection/FaceGeometry.h"
#include "tools/selection/FaceGeometryBuilder.h"
#include "tools/selection/SelectionOutlineRenderer.h"
#include "tools/selection/FaceHighlightRenderer.h"
#include "tools/selection/PushPullTool.h"
#include "tools/selection/PushPullPreviewRenderer.h"
#include "tools/selection/FaceMoveTool.h"
#include "tools/selection/FaceMovePreviewRenderer.h"
#include "tools/selection/Raycaster.h"

#include "tools/transform/TransformController.h"
#include "tools/transform/TransformTypes.h"
#include "tools/transform/TranslateGizmoRenderer.h"
#include "tools/transform/TranslateGizmoSelector.h"
#include "tools/transform/ScaleGizmoRenderer.h"
#include "tools/transform/ScaleGizmoSelector.h"
#include "tools/transform/RotateGizmoRenderer.h"
#include "tools/transform/RotateGizmoSelector.h"

#include "resources/AssetPaths.h"

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

    ObjectSelector objectSelector;
    FaceSelector faceSelector;
    FaceGeometryBuilder faceGeometryBuilder;
    FaceSelection selectedFace;
    SelectionOutlineRenderer selectionOutlineRenderer;
    FaceHighlightRenderer faceHighlightRenderer;

    PushPullTool pushPullTool;
    PushPullPreviewRenderer pushPullPreviewRenderer;

    FaceMoveTool faceMoveTool;
    FaceMovePreviewRenderer faceMovePreviewRenderer;

    TransformController transformController;
    TranslateGizmoRenderer translateGizmoRenderer;
    TranslateGizmoSelector translateGizmoSelector;
    ScaleGizmoRenderer scaleGizmoRenderer;
    ScaleGizmoSelector scaleGizmoSelector;
    RotateGizmoRenderer rotateGizmoRenderer;
    RotateGizmoSelector rotateGizmoSelector;

    Raycaster raycaster;

    Mesh cylinder = PrimitiveFactory::createCone(24, 1.5f, 4.0f);
    Mesh sphere = PrimitiveFactory::createUvSphere(32, 16, 0.8f);
    Mesh ellipsoid = PrimitiveFactory::createEllipsoid(32, 16, 0.7f, 1.1f, 0.5f);

    SceneObject cylinderObject(cylinder);
    SceneObject sphereObject(sphere);
    SceneObject ellipsoidObject(ellipsoid);

    cylinderObject.getTransform().setPosition(glm::vec3(-2.0f, 0.0f, 0.0f));
    sphereObject.getTransform().setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    ellipsoidObject.getTransform().setPosition(glm::vec3(2.0f, 0.0f, 0.0f));

    Scene scene;
    scene.addObject(cylinderObject);
    scene.addObject(sphereObject);
    scene.addObject(ellipsoidObject);

    Shader shader(
        AssetPaths::shader("basic/vertex.glsl"),
        AssetPaths::shader("basic/fragment.glsl")
    );

    Renderer renderer;

    Camera camera;
    CameraController cameraController;

    g_Camera = &camera;
    g_CameraController = &cameraController;

    glfwSetScrollCallback(window.getWindow(), scrollCallback);

    SceneObject* selectedObject = nullptr;
    bool faceModeActive = false;

    bool leftMouseWasPressed = false;
    bool wWasPressed = false;
    bool eWasPressed = false;
    bool rWasPressed = false;
    bool gWasPressed = false;
    bool lWasPressed = false;
    bool fWasPressed = false;
    bool tWasPressed = false;
    bool mWasPressed = false;
    bool escWasPressed = false;

    bool keyWasPressed[512] = { false };

    while (!window.shouldClose())
    {
        renderer.clear();

        if (!transformController.isDragging())
        {
            cameraController.processMouse(window.getWindow(), camera);
        }

        if (pushPullTool.isActive())
        {
            pushPullTool.update(window.getWindow(), camera);
        }

        if (faceMoveTool.isActive())
        {
            faceMoveTool.update(window.getWindow(), camera);
        }

        if (transformController.isDragging())
        {
            Ray dragRay = raycaster.buildRayFromMouse(window.getWindow(), camera);
            transformController.updateDragFromRay(dragRay);
        }

        if (pushPullTool.isActive())
        {
            for (int key = 0; key < 512; key++)
            {
                bool pressed = glfwGetKey(window.getWindow(), key) == GLFW_PRESS;

                if (pressed && !keyWasPressed[key])
                {
                    pushPullTool.onKeyPressed(key);
                }

                keyWasPressed[key] = pressed;
            }
        }
        else if (faceMoveTool.isActive())
        {
            for (int key = 0; key < 512; key++)
            {
                bool pressed = glfwGetKey(window.getWindow(), key) == GLFW_PRESS;

                if (pressed && !keyWasPressed[key])
                {
                    faceMoveTool.onKeyPressed(key);
                }

                keyWasPressed[key] = pressed;
            }
        }
        else
        {
            for (int key = 0; key < 512; key++)
            {
                keyWasPressed[key] = false;
            }
        }

        bool fPressed = glfwGetKey(window.getWindow(), GLFW_KEY_F) == GLFW_PRESS;
        if (fPressed && !fWasPressed)
        {
            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                !transformController.isDragging())
            {
                faceModeActive = !faceModeActive;
                selectedFace.clear();

                if (faceModeActive)
                {
                    std::cout << "[FACE MODE] Ativado\n";
                }
                else
                {
                    std::cout << "[FACE MODE] Desativado\n";
                }
            }
        }
        fWasPressed = fPressed;

        bool leftMousePressed =
            glfwGetMouseButton(window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        if (leftMousePressed && !leftMouseWasPressed)
        {
            bool gizmoHandledClick = false;

            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                selectedObject != nullptr &&
                transformController.hasActiveMode())
            {
                TransformAxis clickedAxis = TransformAxis::None;

                if (transformController.getMode() == TransformMode::Translate)
                {
                    clickedAxis = translateGizmoSelector.selectAxis(
                        *selectedObject,
                        window.getWindow(),
                        camera,
                        transformController.getSpace()
                    );
                }
                else if (transformController.getMode() == TransformMode::Scale)
                {
                    clickedAxis = scaleGizmoSelector.selectAxis(
                        *selectedObject,
                        window.getWindow(),
                        camera,
                        transformController.getSpace()
                    );
                }
                else if (transformController.getMode() == TransformMode::Rotate)
                {
                    clickedAxis = rotateGizmoSelector.selectAxis(
                        *selectedObject,
                        window.getWindow(),
                        camera,
                        transformController.getSpace()
                    );
                }

                if (clickedAxis != TransformAxis::None)
                {
                    transformController.setAxis(clickedAxis);

                    Ray clickRay = raycaster.buildRayFromMouse(window.getWindow(), camera);
                    bool dragStarted = transformController.beginDragFromRay(clickRay);

                    gizmoHandledClick = true;

                    if (clickedAxis == TransformAxis::X)
                    {
                        std::cout << "[GIZMO] Eixo X selecionado\n";
                    }
                    else if (clickedAxis == TransformAxis::Y)
                    {
                        std::cout << "[GIZMO] Eixo Y selecionado\n";
                    }
                    else if (clickedAxis == TransformAxis::Z)
                    {
                        std::cout << "[GIZMO] Eixo Z selecionado\n";
                    }

                    if (!dragStarted)
                    {
                        std::cout << "[GIZMO] Clique consumido, mas o drag nao iniciou\n";
                    }
                }
            }

            if (gizmoHandledClick)
            {
            }
            else if (pushPullTool.isActive())
            {
                bool confirmSuccess = pushPullTool.confirm();

                if (confirmSuccess)
                {
                    std::cout << "[PUSH/PULL] Operacao finalizada\n";
                }
                else
                {
                    std::cout << "[PUSH/PULL] Falha ao confirmar operacao\n";
                }
            }
            else if (faceMoveTool.isActive())
            {
                bool confirmSuccess = faceMoveTool.confirm();

                if (confirmSuccess)
                {
                    std::cout << "[FACE MOVE] Operacao finalizada\n";
                }
                else
                {
                    std::cout << "[FACE MOVE] Falha ao confirmar operacao\n";
                }
            }
            else if (faceModeActive && selectedObject != nullptr)
            {
                int faceIndex = faceSelector.selectFace(*selectedObject, window.getWindow(), camera);

                if (faceIndex != -1)
                {
                    selectedFace.set(selectedObject, faceIndex);

                    FaceGeometry faceGeometry = faceGeometryBuilder.build(selectedFace);

                    std::cout << "[FACE] Face selecionada: " << faceIndex << "\n";

                    if (faceGeometry.isValid())
                    {
                        glm::vec3 localNormal = faceGeometry.getLocalNormal();

                        std::cout << "[FACE GEOMETRY] Normal local: ("
                            << localNormal.x << ", "
                            << localNormal.y << ", "
                            << localNormal.z << ")\n";
                    }
                    else
                    {
                        std::cout << "[FACE GEOMETRY] Geometria invalida\n";
                    }
                }
                else
                {
                    selectedFace.clear();
                    std::cout << "[FACE] Nenhuma face selecionada\n";
                }
            }
            else
            {
                selectedObject = objectSelector.selectObject(window.getWindow(), camera, scene);
                transformController.setSelectedObject(selectedObject);
                selectedFace.clear();

                if (selectedObject != nullptr)
                {
                    std::cout << "[SELECT] Objeto selecionado\n";
                }
                else
                {
                    std::cout << "[SELECT] Nenhum objeto selecionado\n";
                }
            }
        }

        if (!leftMousePressed && leftMouseWasPressed)
        {
            if (transformController.isDragging())
            {
                transformController.endDrag();
            }
        }

        leftMouseWasPressed = leftMousePressed;

        bool wPressed = glfwGetKey(window.getWindow(), GLFW_KEY_W) == GLFW_PRESS;
        if (wPressed && !wWasPressed)
        {
            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                !transformController.isDragging())
            {
                transformController.setMode(TransformMode::Translate);
                std::cout << "[MODE] Translate\n";
            }
        }
        wWasPressed = wPressed;

        bool ePressed = glfwGetKey(window.getWindow(), GLFW_KEY_E) == GLFW_PRESS;
        if (ePressed && !eWasPressed)
        {
            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                !transformController.isDragging())
            {
                transformController.setMode(TransformMode::Rotate);
                std::cout << "[MODE] Rotate\n";
            }
        }
        eWasPressed = ePressed;

        bool rPressed = glfwGetKey(window.getWindow(), GLFW_KEY_R) == GLFW_PRESS;
        if (rPressed && !rWasPressed)
        {
            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                !transformController.isDragging())
            {
                transformController.setMode(TransformMode::Scale);
                std::cout << "[MODE] Scale\n";
            }
        }
        rWasPressed = rPressed;

        bool gPressed = glfwGetKey(window.getWindow(), GLFW_KEY_G) == GLFW_PRESS;
        if (gPressed && !gWasPressed)
        {
            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                !transformController.isDragging())
            {
                transformController.setSpace(TransformSpace::Global);
                std::cout << "[SPACE] Global\n";
            }
        }
        gWasPressed = gPressed;

        bool lPressed = glfwGetKey(window.getWindow(), GLFW_KEY_L) == GLFW_PRESS;
        if (lPressed && !lWasPressed)
        {
            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                !transformController.isDragging())
            {
                transformController.setSpace(TransformSpace::Local);
                std::cout << "[SPACE] Local\n";
            }
        }
        lWasPressed = lPressed;

        bool tPressed = glfwGetKey(window.getWindow(), GLFW_KEY_T) == GLFW_PRESS;
        if (tPressed && !tWasPressed)
        {
            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                !transformController.isDragging())
            {
                if (selectedFace.isValid())
                {
                    bool started = pushPullTool.start(selectedFace, window.getWindow(), camera);

                    if (!started)
                    {
                        std::cout << "[PUSH/PULL] Falha ao iniciar ferramenta\n";
                    }
                }
                else
                {
                    std::cout << "[PUSH/PULL] Nenhuma face selecionada\n";
                }
            }
        }
        tWasPressed = tPressed;

        bool mPressed = glfwGetKey(window.getWindow(), GLFW_KEY_M) == GLFW_PRESS;
        if (mPressed && !mWasPressed)
        {
            if (!pushPullTool.isActive() &&
                !faceMoveTool.isActive() &&
                !transformController.isDragging())
            {
                if (selectedFace.isValid())
                {
                    bool started = faceMoveTool.start(selectedFace, window.getWindow(), camera);

                    if (!started)
                    {
                        std::cout << "[FACE MOVE] Falha ao iniciar ferramenta\n";
                    }
                }
                else
                {
                    std::cout << "[FACE MOVE] Nenhuma face selecionada\n";
                }
            }
        }
        mWasPressed = mPressed;

        bool escPressed = glfwGetKey(window.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS;
        if (escPressed && !escWasPressed)
        {
            if (pushPullTool.isActive())
            {
                pushPullTool.cancel();
                std::cout << "[PUSH/PULL] Cancelado\n";
            }
            else if (faceMoveTool.isActive())
            {
                faceMoveTool.cancel();
                std::cout << "[FACE MOVE] Cancelado\n";
            }
            else if (transformController.isDragging())
            {
                transformController.endDrag();
                std::cout << "[GIZMO] Drag cancelado\n";
            }
            else
            {
                faceModeActive = false;
                selectedFace.clear();
                transformController.setMode(TransformMode::None);
                transformController.clearAxis();
                transformController.setSpace(TransformSpace::Global);

                std::cout << "[CLEAR] Modo, eixo, espaco e selecao de face resetados\n";
            }
        }
        escWasPressed = escPressed;

        renderer.renderScene(scene, camera, shader);
        gridRenderer.render(camera);
        axisRenderer.render(camera);

        pushPullPreviewRenderer.render(pushPullTool, camera);
        faceMovePreviewRenderer.render(faceMoveTool, camera);

        if (selectedObject != nullptr)
        {
            selectionOutlineRenderer.render(*selectedObject, camera);

            if (!pushPullTool.isActive() && !faceMoveTool.isActive())
            {
                if (transformController.getMode() == TransformMode::Translate)
                {
                    translateGizmoRenderer.render(
                        *selectedObject,
                        camera,
                        transformController.getAxis(),
                        transformController.getSpace()
                    );
                }
                else if (transformController.getMode() == TransformMode::Scale)
                {
                    scaleGizmoRenderer.render(
                        *selectedObject,
                        camera,
                        transformController.getAxis(),
                        transformController.getSpace()
                    );
                }
                else if (transformController.getMode() == TransformMode::Rotate)
                {
                    rotateGizmoRenderer.render(
                        *selectedObject,
                        camera,
                        transformController.getAxis(),
                        transformController.getSpace()
                    );
                }
            }

            if (selectedFace.isValid())
            {
                faceHighlightRenderer.render(
                    *selectedFace.getObject(),
                    selectedFace.getFaceIndex(),
                    camera
                );
            }
        }

        window.pollEvents();
        window.swapBuffers();
    }

    return 0;
}