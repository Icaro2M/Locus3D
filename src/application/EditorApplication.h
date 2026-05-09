#pragma once

#include "../core/WindowManager.h"
#include "../graphics/Shader.h"

#include "EditorState.h"
#include "AppEventBus.h"
#include "InputHandler.h"
#include "ToolManager.h"

#include "controllers/SelectionController.h"
#include "controllers/TransformBridge.h"
#include "controllers/FaceToolController.h"
#include "controllers/RenderCoordinator.h"
#include "controllers/CameraContext.h"
#include "controllers/SceneContext.h"

#include "../ui/UIContext.h"
#include "../tools/selection/Raycaster.h"

#include <cstdint>

class EditorApplication
{
public:
    EditorApplication(WindowManager* window);

    void processInput();
    void update();
    void render();

    AppEventBus* getEventBus()
    {
        return &m_eventBus;
    }

    UIContext* getUIContext()
    {
        return &m_uiContext;
    }

    CameraContext& getCameraContext()
    {
        return m_cameraContext;
    }

private:
    WindowManager* m_window;

    AppEventBus m_eventBus;
    EditorState m_editorState;
    UIContext m_uiContext;

    InputHandler m_inputHandler;
    ToolManager m_toolManager;
    SelectionController m_selectionController;
    TransformBridge m_transformBridge;
    FaceToolController m_faceToolController;
    RenderCoordinator m_renderCoordinator;
    CameraContext m_cameraContext;
    SceneContext m_sceneContext;

    Raycaster m_raycaster;

    Shader m_shader;

    uint32_t m_lastSyncedSelectedObjectId = 0;

    void setupEventSubscriptions();
    void syncUI();
    void clearFaceEditingState();
    void selectCreatedObject(SceneObject* object);
};