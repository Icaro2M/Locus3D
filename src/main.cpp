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
	ObjectSelector selector;
	SelectionOutlineRenderer selectionOutlineRenderer;
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
	bool leftMouseWasPressed = false;

	bool wWasPressed = false;
	bool eWasPressed = false;
	bool rWasPressed = false;
	bool xWasPressed = false;
	bool yWasPressed = false;
	bool zWasPressed = false;
	bool escWasPressed = false;
	bool positiveWasPressed = false;
	bool negativeWasPressed = false;

	while (!window.shouldClose())
	{
		renderer.clear();

		cameraController.processMouse(window.getWindow(), camera);

		bool leftMousePressed = glfwGetMouseButton(window.getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
		if (leftMousePressed && !leftMouseWasPressed)
		{
			selectedObject = selector.selectObject(window.getWindow(), camera, scene);
			transformController.setSelectedObject(selectedObject);
		}
		leftMouseWasPressed = leftMousePressed;

		bool wPressed = glfwGetKey(window.getWindow(), GLFW_KEY_W) == GLFW_PRESS;
		if (wPressed && !wWasPressed)
		{
			transformController.setMode(TransformMode::Translate);
		}
		wWasPressed = wPressed;

		bool ePressed = glfwGetKey(window.getWindow(), GLFW_KEY_E) == GLFW_PRESS;
		if (ePressed && !eWasPressed)
		{
			transformController.setMode(TransformMode::Rotate);
		}
		eWasPressed = ePressed;

		bool rPressed = glfwGetKey(window.getWindow(), GLFW_KEY_R) == GLFW_PRESS;
		if (rPressed && !rWasPressed)
		{
			transformController.setMode(TransformMode::Scale);
		}
		rWasPressed = rPressed;

		bool xPressed = glfwGetKey(window.getWindow(), GLFW_KEY_X) == GLFW_PRESS;
		if (xPressed && !xWasPressed)
		{
			transformController.setAxis(TransformAxis::X);
		}
		xWasPressed = xPressed;

		bool yPressed = glfwGetKey(window.getWindow(), GLFW_KEY_Y) == GLFW_PRESS;
		if (yPressed && !yWasPressed)
		{
			transformController.setAxis(TransformAxis::Y);
		}
		yWasPressed = yPressed;

		bool zPressed = glfwGetKey(window.getWindow(), GLFW_KEY_Z) == GLFW_PRESS;
		if (zPressed && !zWasPressed)
		{
			transformController.setAxis(TransformAxis::Z);
		}
		zWasPressed = zPressed;

		bool escPressed = glfwGetKey(window.getWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS;
		if (escPressed && !escWasPressed)
		{
			transformController.setMode(TransformMode::None);
			transformController.setAxis(TransformAxis::None);
		}
		escWasPressed = escPressed;

		bool positivePressed =
			glfwGetKey(window.getWindow(), GLFW_KEY_UP) == GLFW_PRESS ||
			glfwGetKey(window.getWindow(), GLFW_KEY_EQUAL) == GLFW_PRESS;

		if (positivePressed && !positiveWasPressed)
		{
			transformController.applyPositiveStep();
		}
		positiveWasPressed = positivePressed;

		bool negativePressed =
			glfwGetKey(window.getWindow(), GLFW_KEY_DOWN) == GLFW_PRESS ||
			glfwGetKey(window.getWindow(), GLFW_KEY_MINUS) == GLFW_PRESS;

		if (negativePressed && !negativeWasPressed)
		{
			transformController.applyNegativeStep();
		}
		negativeWasPressed = negativePressed;

		renderer.renderScene(scene, camera, shader);

		gridRenderer.render(camera);
		axisRenderer.render(camera);

		if (selectedObject != nullptr)
		{
			selectionOutlineRenderer.render(*selectedObject, camera);
			transformGizmoRenderer.render(*selectedObject, camera);
		}

		window.pollEvents();
		window.swapBuffers();
	}

	return 0;
}