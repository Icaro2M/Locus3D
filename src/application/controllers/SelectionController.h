#pragma once

#include "../EditorState.h"
#include "../AppEventBus.h"
#include "../../scene/Scene.h"
#include "../../scene/Camera.h"
#include "../../tools/selection/ObjectSelector.h"
#include "../../tools/selection/FaceSelector.h"

#include <GLFW/glfw3.h>

class SelectionController
{
public:
    SelectionController(EditorState* state, AppEventBus* eventBus);
    ~SelectionController() = default;

    void handleSelection(GLFWwindow* window, Camera& camera, Scene& scene);

    bool selectFaceUnderMouse(GLFWwindow* window, Camera& camera);

private:
    EditorState* m_state;
    AppEventBus* m_eventBus;

    ObjectSelector m_objectSelector;
    FaceSelector m_faceSelector;
};