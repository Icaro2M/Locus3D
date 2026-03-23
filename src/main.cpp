#include <iostream>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>

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
#include "tools/selection/SelectionOutlineRenderer.h"
#include "tools/selection/FaceHighlightRenderer.h"

#include "tools/transform/TransformController.h"
#include "tools/transform/TransformGizmoRenderer.h"

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
	SelectionOutlineRenderer selectionOutlineRenderer;
	FaceHighlightRenderer faceHighlightRenderer;

	TransformController transformController;
	TransformGizmoRenderer transformGizmoRenderer;

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

	SceneObject* selectedObject = nullptr;
	int selectedFaceIndex = -1;
	bool faceModeActive = false;

	bool leftMouseWasPressed = false;

	bool wWasPressed = false;
	bool eWasPressed = false;
	bool rWasPressed = false;

	bool xWasPressed = false;
	bool yWasPressed = false;
	bool zWasPressed = false;

	bool gWasPressed = false;
	bool lWasPressed = false;
	bool fWasPressed = false;

	bool escWasPressed = false;
	bool positiveWasPressed = false;
	bool negativeWasPressed = false;

	while (!window.shouldClose())
	{
		renderer.clear();

		cameraController.processMouse(window.getWindow(), camera);

		bool fPressed = glfwGetKey(window.getWindow(), GLFW_KEY_F) == GLFW_PRESS;
		if (fPressed && !fWasPressed)
		{
			faceModeActive = !faceModeActive;
			selectedFaceIndex = -1;

			if (faceModeActive)
			{
				std::cout << "[FACE MODE] Ativado\n";
			}
			else
			{
				std::cout << "[FACE MODE] Desativado\n";
			}
		}
		fWasPressed = fPressed;

		bool leftMousePressed = glfwGetMouseButton(window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		if (leftMousePressed && !leftMouseWasPressed)
		{
			if (faceModeActive && selectedObject != nullptr)
			{
				selectedFaceIndex = faceSelector.selectFace(*selectedObject, window.getWindow(), camera);

				if (selectedFaceIndex != -1)
				{
					std::cout << "[FACE] Face selecionada: " << selectedFaceIndex << "\n";
				}
				else
				{
					std::cout << "[FACE] Nenhuma face selecionada\n";
				}
			}
			else
			{
				selectedObject = objectSelector.selectObject(window.getWindow(), camera, scene);
				transformController.setSelectedObject(selectedObject);
				selectedFaceIndex = -1;

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
		leftMouseWasPressed = leftMousePressed;

		bool wPressed = glfwGetKey(window.getWindow(), GLFW_KEY_W) == GLFW_PRESS;
		if (wPressed && !wWasPressed)
		{
			transformController.setMode(TransformMode::Translate);
			std::cout << "[MODE] Translate\n";
		}
		wWasPressed = wPressed;

		bool ePressed = glfwGetKey(window.getWindow(), GLFW_KEY_E) == GLFW_PRESS;
		if (ePressed && !eWasPressed)
		{
			transformController.setMode(TransformMode::Rotate);
			std::cout << "[MODE] Rotate\n";
		}
		eWasPressed = ePressed;

		bool rPressed = glfwGetKey(window.getWindow(), GLFW_KEY_R) == GLFW_PRESS;
		if (rPressed && !rWasPressed)
		{
			transformController.setMode(TransformMode::Scale);
			std::cout << "[MODE] Scale\n";
		}
		rWasPressed = rPressed;

		bool xPressed = glfwGetKey(window.getWindow(), GLFW_KEY_X) == GLFW_PRESS;
		if (xPressed && !xWasPressed)
		{
			transformController.setAxis(TransformAxis::X);
			std::cout << "[AXIS] X\n";
		}
		xWasPressed = xPressed;

		bool yPressed = glfwGetKey(window.getWindow(), GLFW_KEY_Y) == GLFW_PRESS;
		if (yPressed && !yWasPressed)
		{
			transformController.setAxis(TransformAxis::Y);
			std::cout << "[AXIS] Y\n";
		}
		yWasPressed = yPressed;

		bool zPressed = glfwGetKey(window.getWindow(), GLFW_KEY_Z) == GLFW_PRESS;
		if (zPressed && !zWasPressed)
		{
			transformController.setAxis(TransformAxis::Z);
			std::cout << "[AXIS] Z\n";
		}
		zWasPressed = zPressed;

		bool gPressed = glfwGetKey(window.getWindow(), GLFW_KEY_G) == GLFW_PRESS;
		if (gPressed && !gWasPressed)
		{
			transformController.setSpace(TransformSpace::Global);
			std::cout << "[SPACE] Global\n";
		}
		gWasPressed = gPressed;

		bool lPressed = glfwGetKey(window.getWindow(), GLFW_KEY_L) == GLFW_PRESS;
		if (lPressed && !lWasPressed)
		{
			transformController.setSpace(TransformSpace::Local);
			std::cout << "[SPACE] Local\n";
		}
		lWasPressed = lPressed;

		bool escPressed = glfwGetKey(window.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS;
		if (escPressed && !escWasPressed)
		{
			faceModeActive = false;
			selectedFaceIndex = -1;
			transformController.setMode(TransformMode::None);
			transformController.setAxis(TransformAxis::None);
			transformController.setSpace(TransformSpace::Global);

			std::cout << "[CLEAR] Modo, eixo, espaco e selecao de face resetados\n";
		}
		escWasPressed = escPressed;

		bool positivePressed =
			glfwGetKey(window.getWindow(), GLFW_KEY_UP) == GLFW_PRESS ||
			glfwGetKey(window.getWindow(), GLFW_KEY_EQUAL) == GLFW_PRESS;

		if (positivePressed && !positiveWasPressed)
		{
			transformController.applyPositiveStep();
			std::cout << "[STEP] Positivo\n";
		}
		positiveWasPressed = positivePressed;

		bool negativePressed =
			glfwGetKey(window.getWindow(), GLFW_KEY_DOWN) == GLFW_PRESS ||
			glfwGetKey(window.getWindow(), GLFW_KEY_MINUS) == GLFW_PRESS;

		if (negativePressed && !negativeWasPressed)
		{
			transformController.applyNegativeStep();
			std::cout << "[STEP] Negativo\n";
		}
		negativeWasPressed = negativePressed;

		renderer.renderScene(scene, camera, shader);

		gridRenderer.render(camera);
		axisRenderer.render(camera);

		if (selectedObject != nullptr)
		{
			selectionOutlineRenderer.render(*selectedObject, camera);

			transformGizmoRenderer.render(
				*selectedObject,
				camera,
				transformController.getAxis(),
				transformController.getSpace()
			);

			if (selectedFaceIndex != -1)
			{
				faceHighlightRenderer.render(*selectedObject, selectedFaceIndex, camera);
			}
		}

		window.pollEvents();
		window.swapBuffers();
	}

	return 0;
}