#pragma once

#include "../EditorState.h"
#include "../AppEventBus.h"
#include "../../scene/Camera.h"
#include "../../tools/transform/TransformController.h"
#include "../../tools/transform/TranslateGizmoSelector.h"
#include "../../tools/transform/RotateGizmoSelector.h"
#include "../../tools/transform/ScaleGizmoSelector.h"
#include "../../tools/selection/Raycaster.h"
#include <GLFW/glfw3.h>


class TransformBridge {
public:
    TransformBridge(EditorState* state, AppEventBus* eventBus);
    ~TransformBridge() = default;

    void handleInputEvent(EventType eventType);
    
    bool handleMouseClick(GLFWwindow* window, Camera& camera, Raycaster& raycaster);
    void handleMouseMove(GLFWwindow* window, Camera& camera, Raycaster& raycaster);
    void handleMouseRelease();

    TransformController& getController();

private:
    EditorState* m_state;
    AppEventBus* m_eventBus;

    TransformController m_transformController;
    TranslateGizmoSelector m_translateSelector;
    RotateGizmoSelector m_rotateSelector;
    ScaleGizmoSelector m_scaleSelector;
};