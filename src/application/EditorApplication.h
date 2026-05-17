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
#include "controllers/WindowController.h"

#include "clipboard/ObjectClipboard.h"

#include "history/UndoRedoManager.h"
#include "history/EditorSceneSnapshotBuilder.h"

#include "../ui/bridge/UIContext.h"
#include "../tools/selection/Raycaster.h"

#include <cstdint>
#include <string>

class EditorApplication
{
public:
    EditorApplication(WindowManager* window);

    void processInput();
    void update();
    void render();

    bool saveScene();
    bool saveSceneAs();
    bool loadSceneFromDialog();
    bool saveSceneToFile(const std::string& filePath);
    bool loadSceneFromFile(const std::string& filePath);

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

    WindowController* getWindowController()
    {
        return &m_windowController;
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
    WindowController m_windowController;
    SceneContext m_sceneContext;
    ObjectClipboard m_objectClipboard;
    UndoRedoManager m_undoRedoManager;
    

    Raycaster m_raycaster;

    Shader m_shader;

    uint32_t m_lastSyncedSelectedObjectId = 0;

    std::string m_currentScenePath;
    std::string m_currentSceneName = "Novo Projeto";

    void setupEventSubscriptions();
    void syncUI();
    void clearFaceEditingState();
    void selectCreatedObject(SceneObject* object);

    void handleClipboardShortcuts();
    void copySelectedObject();
    void pasteCopiedObject();

    void handleHistoryShortcuts();
    void pushUndoSnapshot();
    bool restoreHistorySnapshot(const EditorSceneSnapshot& snapshot);

    void switchToObjectTransform(EventType transformEventType);
    bool isTransformModeEvent(EventType eventType) const;

    void handleFileShortcuts();

    std::string extractFileName(const std::string& filePath) const;

    const std::string& getCurrentScenePath() const;
    const std::string& getCurrentSceneName() const;
    bool hasScenePath() const;


};
