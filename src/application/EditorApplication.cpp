#include "EditorApplication.h"
#include "../resources/AssetPaths.h"
#include "../platform/NativeFileDialog.h"

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glad/glad.h>

EditorApplication::EditorApplication(WindowManager* window)
    : m_window(window),
    m_shader(
        AssetPaths::shader("basic/vertex.glsl"),
        AssetPaths::shader("basic/fragment.glsl")
    ),
    m_toolManager(&m_editorState, &m_eventBus),
    m_selectionController(&m_editorState, &m_eventBus),
    m_transformBridge(&m_editorState, &m_eventBus),
    m_faceToolController(&m_editorState),
    m_renderCoordinator(&m_editorState),
    m_undoRedoManager(12)
{
    setupEventSubscriptions();
}

void EditorApplication::processInput()
{
    m_inputHandler.process(m_window->getWindow(), &m_eventBus);
    handleFileShortcuts();
    handleHistoryShortcuts();
    handleClipboardShortcuts();

    static bool wasLeftMouseDown = false;

    bool isLeftMouseDown =
        glfwGetMouseButton(m_window->getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    bool mouseClicked = isLeftMouseDown && !wasLeftMouseDown;
    bool mouseReleased = !isLeftMouseDown && wasLeftMouseDown;

    wasLeftMouseDown = isLeftMouseDown;

    ImGuiIO& io = ImGui::GetIO();

    if (mouseClicked && !io.WantCaptureMouse)
    {
        if (m_faceToolController.hasRunningTool())
        {
            EditorSceneSnapshot beforeConfirm = EditorSceneSnapshotBuilder::capture(
                m_sceneContext,
                m_editorState
            );

            m_faceToolController.confirmActiveTool();

            m_undoRedoManager.pushUndo(std::move(beforeConfirm));
        }
        else if (m_editorState.getActiveTool() != EditorToolType::None)
        {
            bool selectedFace = m_selectionController.selectFaceUnderMouse(
                m_window->getWindow(),
                m_cameraContext.getCamera()
            );

            if (selectedFace)
            {
                m_editorState.clearHoveredFace();

                m_faceToolController.startActiveTool(
                    m_window->getWindow(),
                    m_cameraContext.getCamera()
                );
            }
        }
        else
        {
            EditorSceneSnapshot beforeTransform = EditorSceneSnapshotBuilder::capture(
                m_sceneContext,
                m_editorState
            );

            bool startedTransform = m_transformBridge.handleMouseClick(
                m_window->getWindow(),
                m_cameraContext.getCamera(),
                m_raycaster
            );

            if (startedTransform)
            {
                m_undoRedoManager.pushUndo(std::move(beforeTransform));
            }
            else
            {
                m_selectionController.handleSelection(
                    m_window->getWindow(),
                    m_cameraContext.getCamera(),
                    m_sceneContext.getScene()
                );
            }
        }
    }

    if (mouseReleased)
    {
        m_transformBridge.handleMouseRelease();
    }

    static bool wasDeletePressed = false;

    bool isDeletePressed =
        glfwGetKey(m_window->getWindow(), GLFW_KEY_DELETE) == GLFW_PRESS;

    if (isDeletePressed && !wasDeletePressed && !io.WantTextInput)
    {
        SceneObject* selected = m_editorState.getSelectedObject();

        if (selected != nullptr)
        {
            m_eventBus.emit(
                EventType::DeleteObject,
                static_cast<int>(selected->getId())
            );
        }
    }

    wasDeletePressed = isDeletePressed;

    static bool wasW = false;
    static bool wasE = false;
    static bool wasR = false;

    bool isW = glfwGetKey(m_window->getWindow(), GLFW_KEY_W) == GLFW_PRESS;
    bool isE = glfwGetKey(m_window->getWindow(), GLFW_KEY_E) == GLFW_PRESS;
    bool isR = glfwGetKey(m_window->getWindow(), GLFW_KEY_R) == GLFW_PRESS;

    bool canUseTransformShortcuts =
        !io.WantTextInput &&
        m_editorState.getActiveTool() == EditorToolType::None &&
        !m_faceToolController.hasRunningTool();

    if (isW && !wasW && canUseTransformShortcuts)
    {
        m_uiContext.activeTransformMode = TransformMode::Translate;
        m_eventBus.emit(EventType::InputKeyW, 0);
    }

    if (isE && !wasE && canUseTransformShortcuts)
    {
        m_uiContext.activeTransformMode = TransformMode::Rotate;
        m_eventBus.emit(EventType::InputKeyE, 0);
    }

    if (isR && !wasR && canUseTransformShortcuts)
    {
        m_uiContext.activeTransformMode = TransformMode::Scale;
        m_eventBus.emit(EventType::InputKeyR, 0);
    }

    wasW = isW;
    wasE = isE;
    wasR = isR;
}

void EditorApplication::update()
{
    int framebufferWidth = 0;
    int framebufferHeight = 0;

    glfwGetFramebufferSize(
        m_window->getWindow(),
        &framebufferWidth,
        &framebufferHeight
    );

    static int lastFramebufferWidth = 0;
    static int lastFramebufferHeight = 0;

    if (framebufferWidth > 0 &&
        framebufferHeight > 0 &&
        (framebufferWidth != lastFramebufferWidth ||
            framebufferHeight != lastFramebufferHeight))
    {
        glViewport(0, 0, framebufferWidth, framebufferHeight);

        m_cameraContext.resize(framebufferWidth, framebufferHeight);

        lastFramebufferWidth = framebufferWidth;
        lastFramebufferHeight = framebufferHeight;
    }

    if (!m_transformBridge.getController().isDragging() &&
        !m_faceToolController.hasRunningTool())
    {
        m_cameraContext.update(m_window->getWindow());
    }

    m_transformBridge.handleMouseMove(
        m_window->getWindow(),
        m_cameraContext.getCamera(),
        m_raycaster
    );

    m_faceToolController.update(
        m_window->getWindow(),
        m_cameraContext.getCamera()
    );

    if (m_editorState.getActiveTool() != EditorToolType::None &&
        !m_faceToolController.hasRunningTool() &&
        m_editorState.getSelectedObject() != nullptr)
    {
        m_selectionController.updateHoveredFace(
            m_window->getWindow(),
            m_cameraContext.getCamera()
        );
    }
    else
    {
        m_editorState.clearHoveredFace();
    }

    syncUI();
}

void EditorApplication::render()
{
    m_renderCoordinator.render(
        m_sceneContext.getScene(),
        m_cameraContext.getCamera(),
        m_shader,
        m_faceToolController,
        m_transformBridge.getController()
    );
}

void EditorApplication::setupEventSubscriptions()
{
    m_eventBus.subscribe([this](const Event& e)
        {
            if (e.type == EventType::FileOpen)
            {
                loadSceneFromDialog();
            }
            else if (e.type == EventType::FileSave)
            {
                saveScene();
            }
            else if (e.type == EventType::FileSaveAs)
            {
                saveSceneAs();
            }
            else if (e.type == EventType::InputKeyW ||
                e.type == EventType::InputKeyE ||
                e.type == EventType::InputKeyR ||
                e.type == EventType::InputKeyG ||
                e.type == EventType::InputKeyL)
            {
                if (m_editorState.getActiveTool() == EditorToolType::None && !m_faceToolController.hasRunningTool())
                {
                    m_transformBridge.handleInputEvent(e.type);
                }
            }
            else if (e.type == EventType::InputKeyEscape)
            {
                if (m_faceToolController.hasRunningTool())
                {
                    m_faceToolController.cancelActiveTool();
                    m_editorState.clearHoveredFace();
                    m_editorState.setFaceModeActive(true);
                    m_uiContext.isFaceModeActive = true;
                }
                else if (m_editorState.getActiveTool() != EditorToolType::None)
                {
                    clearFaceEditingState();
                }
                else
                {
                    m_transformBridge.handleInputEvent(e.type);
                    clearFaceEditingState();
                }
            }
            else if (e.type == EventType::InputKeyF)
            {
            }
            else if (e.type == EventType::InputKeyT ||
                e.type == EventType::InputKeyM ||
                e.type == EventType::InputKeyS)
            {
                if (m_editorState.getSelectedObject() != nullptr)
                {
                    m_toolManager.handleInputEvent(e.type);
                    m_uiContext.isFaceModeActive = m_editorState.isFaceModeActive();
                }
            }
            else if (e.type == EventType::AddPrimitive)
            {
                pushUndoSnapshot();
                clearFaceEditingState();

                m_sceneContext.addPrimitive(e.payloadInt);

                auto& objects = m_sceneContext.getScene().getObjects();
                if (!objects.empty())
                {
                    selectCreatedObject(objects.back());
                }
            }
            else if (e.type == EventType::AddCustomSolid)
            {
                pushUndoSnapshot();
                clearFaceEditingState();

                m_sceneContext.addCustomSolid(
                    m_uiContext.customSolidName,
                    m_uiContext.customSolidSides,
                    m_uiContext.customSolidBottomRadius,
                    m_uiContext.customSolidTopRadius,
                    m_uiContext.customSolidHeight
                );

                auto& objects = m_sceneContext.getScene().getObjects();
                if (!objects.empty())
                {
                    selectCreatedObject(objects.back());
                }
            }
            else if (e.type == EventType::TransformChanged)
            {
                if (!m_transformBridge.getController().isDragging())
                {
                    SceneObject* selected = m_editorState.getSelectedObject();

                    if (selected && selected->getId() == m_uiContext.selectedObjectId)
                    {
                        selected->getTransform().setPosition(glm::vec3(
                            m_uiContext.position[0],
                            m_uiContext.position[1],
                            m_uiContext.position[2]
                        ));

                        selected->getTransform().setRotation(glm::vec3(
                            m_uiContext.rotation[0],
                            m_uiContext.rotation[1],
                            m_uiContext.rotation[2]
                        ));

                        selected->getTransform().setScale(glm::vec3(
                            m_uiContext.scale[0],
                            m_uiContext.scale[1],
                            m_uiContext.scale[2]
                        ));
                    }
                }
            }
            else if (e.type == EventType::DeleteObject)
            {
                pushUndoSnapshot();

                uint32_t idToDel = static_cast<uint32_t>(e.payloadInt);

                if (m_editorState.getSelectedObject() != nullptr &&
                    m_editorState.getSelectedObject()->getId() == idToDel)
                {
                    clearFaceEditingState();

                    m_editorState.setSelectedObject(nullptr);
                    m_uiContext.selectedObjectId = 0;
                    m_lastSyncedSelectedObjectId = 0;

                    m_transformBridge.handleInputEvent(EventType::InputKeyEscape);
                }

                m_sceneContext.removeObject(idToDel);
            }
            else if (e.type == EventType::ToolStarted)
            {
                m_uiContext.isFaceModeActive = m_editorState.isFaceModeActive();
            }
            else if (e.type == EventType::ToolCanceled)
            {
                if (m_faceToolController.hasRunningTool())
                {
                    m_faceToolController.cancelActiveTool();
                }
            }
            else if (e.type == EventType::ToolConfirmed)
            {
                if (m_faceToolController.hasRunningTool())
                {
                    m_faceToolController.confirmActiveTool();
                }
            }
        });
}

void EditorApplication::syncUI()
{
    bool inspectorChangedSelection =
        m_uiContext.selectedObjectId != m_lastSyncedSelectedObjectId;

    if (inspectorChangedSelection)
    {
        uint32_t requestedId = m_uiContext.selectedObjectId;

        clearFaceEditingState();

        if (requestedId == 0)
        {
            m_editorState.setSelectedObject(nullptr);
        }
        else
        {
            SceneObject* objectToSelect = nullptr;

            auto& objects = m_sceneContext.getScene().getObjects();

            for (SceneObject* obj : objects)
            {
                if (obj != nullptr && obj->getId() == requestedId)
                {
                    objectToSelect = obj;
                    break;
                }
            }

            m_editorState.setSelectedObject(objectToSelect);

            if (objectToSelect == nullptr)
            {
                m_uiContext.selectedObjectId = 0;
            }
        }
    }

    m_uiContext.sceneObjects.clear();

    auto& objects = m_sceneContext.getScene().getObjects();

    for (SceneObject* obj : objects)
    {
        if (obj == nullptr)
        {
            continue;
        }

        SceneObjectInfo info;
        info.id = obj->getId();
        info.name = obj->getName();
        info.isSelected = (m_editorState.getSelectedObject() == obj);

        m_uiContext.sceneObjects.push_back(info);
    }

    SceneObject* selected = m_editorState.getSelectedObject();

    if (selected != nullptr)
    {
        m_uiContext.selectedObjectId = selected->getId();

        glm::vec3 pos = selected->getTransform().getPosition();
        m_uiContext.position[0] = pos.x;
        m_uiContext.position[1] = pos.y;
        m_uiContext.position[2] = pos.z;

        glm::vec3 rot = selected->getTransform().getRotation();
        m_uiContext.rotation[0] = rot.x;
        m_uiContext.rotation[1] = rot.y;
        m_uiContext.rotation[2] = rot.z;

        glm::vec3 scale = selected->getTransform().getScale();
        m_uiContext.scale[0] = scale.x;
        m_uiContext.scale[1] = scale.y;
        m_uiContext.scale[2] = scale.z;
    }
    else
    {
        m_uiContext.selectedObjectId = 0;
        clearFaceEditingState();
    }

    switch (m_editorState.getActiveTool())
    {
    case EditorToolType::PushPull:
        m_uiContext.activeToolId = 1;
        break;

    case EditorToolType::FaceMove:
        m_uiContext.activeToolId = 2;
        break;

    case EditorToolType::FaceScale:
        m_uiContext.activeToolId = 3;
        break;

    case EditorToolType::None:
    default:
        m_uiContext.activeToolId = 0;
        break;
    }

    m_uiContext.isFaceModeActive = m_editorState.isFaceModeActive();
    m_lastSyncedSelectedObjectId = m_uiContext.selectedObjectId;
}

void EditorApplication::clearFaceEditingState()
{
    if (m_faceToolController.hasRunningTool())
    {
        m_faceToolController.cancelActiveTool();
    }

    m_editorState.clearSelectedFace();
    m_editorState.clearHoveredFace();
    m_editorState.setActiveTool(EditorToolType::None);
    m_editorState.setFaceModeActive(false);

    m_uiContext.isFaceModeActive = false;
    m_uiContext.activeToolId = 0;
}

void EditorApplication::selectCreatedObject(SceneObject* object)
{
    if (object == nullptr)
    {
        return;
    }

    m_editorState.setSelectedObject(object);

    m_uiContext.selectedObjectId = object->getId();
    m_lastSyncedSelectedObjectId = object->getId();

    m_editorState.setTransformMode(TransformMode::Translate);
    m_editorState.setTransformAxis(TransformAxis::None);
    m_uiContext.activeTransformMode = TransformMode::Translate;

    m_transformBridge.handleInputEvent(EventType::InputKeyW);
}

void EditorApplication::handleFileShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
    {
        return;
    }

    GLFWwindow* window = m_window->getWindow();

    bool ctrlPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    bool shiftPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    static bool wasCtrlS = false;
    static bool wasCtrlShiftS = false;
    static bool wasCtrlO = false;

    bool isS = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    bool isO = glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS;

    bool ctrlS = ctrlPressed && !shiftPressed && isS;
    bool ctrlShiftS = ctrlPressed && shiftPressed && isS;
    bool ctrlO = ctrlPressed && isO;

    if (ctrlS && !wasCtrlS)
    {
        saveScene();
    }

    if (ctrlShiftS && !wasCtrlShiftS)
    {
        saveSceneAs();
    }

    if (ctrlO && !wasCtrlO)
    {
        loadSceneFromDialog();
    }

    wasCtrlS = ctrlS;
    wasCtrlShiftS = ctrlShiftS;
    wasCtrlO = ctrlO;
}

bool EditorApplication::saveScene()
{
    if (m_currentScenePath.empty())
    {
        return saveSceneAs();
    }

    return saveSceneToFile(m_currentScenePath);
}

bool EditorApplication::saveSceneAs()
{
    std::string filePath = NativeFileDialog::openSaveDialog();

    if (filePath.empty())
    {
        return false;
    }

    return saveSceneToFile(filePath);
}

bool EditorApplication::loadSceneFromDialog()
{
    std::string filePath = NativeFileDialog::openLoadDialog();

    if (filePath.empty())
    {
        return false;
    }

    return loadSceneFromFile(filePath);
}

bool EditorApplication::saveSceneToFile(const std::string& filePath)
{
    bool saved = m_sceneContext.saveToFile(filePath);

    if (saved)
    {
        m_currentScenePath = filePath;
        m_currentSceneName = extractFileName(filePath);
    }

    return saved;
}

bool EditorApplication::loadSceneFromFile(const std::string& filePath)
{
    clearFaceEditingState();

    m_editorState.setSelectedObject(nullptr);
    m_uiContext.selectedObjectId = 0;
    m_lastSyncedSelectedObjectId = 0;

    m_transformBridge.handleInputEvent(EventType::InputKeyEscape);

    bool loaded = m_sceneContext.loadFromFile(filePath);

    if (loaded)
    {
        m_undoRedoManager.clear();
        m_currentScenePath = filePath;
        m_currentSceneName = extractFileName(filePath);
        syncUI();
    }

    return loaded;
}

std::string EditorApplication::extractFileName(const std::string& filePath) const
{
    size_t slashPosition = filePath.find_last_of("\\/");

    if (slashPosition == std::string::npos)
    {
        return filePath;
    }

    return filePath.substr(slashPosition + 1);
}

const std::string& EditorApplication::getCurrentScenePath() const
{
    return m_currentScenePath;
}

const std::string& EditorApplication::getCurrentSceneName() const
{
    return m_currentSceneName;
}

bool EditorApplication::hasScenePath() const
{
    return !m_currentScenePath.empty();
}

void EditorApplication::handleClipboardShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
    {
        return;
    }

    GLFWwindow* window = m_window->getWindow();

    bool ctrlPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    bool shiftPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    static bool wasCtrlC = false;
    static bool wasCtrlV = false;

    bool isC = glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS;
    bool isV = glfwGetKey(window, GLFW_KEY_V) == GLFW_PRESS;

    bool ctrlC = ctrlPressed && !shiftPressed && isC;
    bool ctrlV = ctrlPressed && !shiftPressed && isV;

    if (ctrlC && !wasCtrlC)
    {
        copySelectedObject();
    }

    if (ctrlV && !wasCtrlV)
    {
        pasteCopiedObject();
    }

    wasCtrlC = ctrlC;
    wasCtrlV = ctrlV;
}

void EditorApplication::copySelectedObject()
{
    SceneObject* selected = m_editorState.getSelectedObject();

    if (selected == nullptr)
    {
        return;
    }

    m_objectClipboard.copyFrom(*selected);
}

void EditorApplication::pasteCopiedObject()
{
    if (!m_objectClipboard.hasData())
    {
        return;
    }

    pushUndoSnapshot();

    clearFaceEditingState();

    SceneObject* pastedObject = m_objectClipboard.pasteInto(m_sceneContext);

    if (pastedObject == nullptr)
    {
        return;
    }

    selectCreatedObject(pastedObject);
    syncUI();
}

void EditorApplication::pushUndoSnapshot()
{
    EditorSceneSnapshot snapshot = EditorSceneSnapshotBuilder::capture(
        m_sceneContext,
        m_editorState
    );

    m_undoRedoManager.pushUndo(std::move(snapshot));
}

bool EditorApplication::restoreHistorySnapshot(const EditorSceneSnapshot& snapshot)
{
    if (m_faceToolController.hasRunningTool())
    {
        m_faceToolController.cancelActiveTool();
    }

    m_transformBridge.getController().endDrag();

    SceneObject* restoredSelectedObject = EditorSceneSnapshotBuilder::restore(
        snapshot,
        m_sceneContext,
        m_editorState
    );

    m_transformBridge.getController().setSelectedObject(restoredSelectedObject);
    m_transformBridge.getController().setMode(m_editorState.getTransformMode());
    m_transformBridge.getController().setSpace(m_editorState.getTransformSpace());
    m_transformBridge.getController().clearAxis();

    if (restoredSelectedObject != nullptr)
    {
        m_uiContext.selectedObjectId = restoredSelectedObject->getId();
        m_lastSyncedSelectedObjectId = restoredSelectedObject->getId();
    }
    else
    {
        m_uiContext.selectedObjectId = 0;
        m_lastSyncedSelectedObjectId = 0;
    }

    m_uiContext.isFaceModeActive = m_editorState.isFaceModeActive();

    switch (m_editorState.getActiveTool())
    {
    case EditorToolType::PushPull:
        m_uiContext.activeToolId = 1;
        break;

    case EditorToolType::FaceMove:
        m_uiContext.activeToolId = 2;
        break;

    case EditorToolType::FaceScale:
        m_uiContext.activeToolId = 3;
        break;

    case EditorToolType::None:
    default:
        m_uiContext.activeToolId = 0;
        break;
    }

    syncUI();

    return true;
}

void EditorApplication::handleHistoryShortcuts()
{
    ImGuiIO& io = ImGui::GetIO();

    if (io.WantTextInput)
    {
        return;
    }

    GLFWwindow* window = m_window->getWindow();

    bool ctrlPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;

    bool shiftPressed =
        glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;

    bool zPressed = glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS;
    bool yPressed = glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS;

    bool ctrlZ = ctrlPressed && !shiftPressed && zPressed;
    bool ctrlY = ctrlPressed && !shiftPressed && yPressed;
    bool ctrlShiftZ = ctrlPressed && shiftPressed && zPressed;

    static bool wasCtrlZ = false;
    static bool wasCtrlY = false;
    static bool wasCtrlShiftZ = false;

    if (ctrlZ && !wasCtrlZ)
    {
        EditorSceneSnapshot currentSnapshot = EditorSceneSnapshotBuilder::capture(
            m_sceneContext,
            m_editorState
        );

        EditorSceneSnapshot snapshotToRestore;

        if (m_undoRedoManager.undo(std::move(currentSnapshot), snapshotToRestore))
        {
            restoreHistorySnapshot(snapshotToRestore);
        }
    }

    if ((ctrlY && !wasCtrlY) || (ctrlShiftZ && !wasCtrlShiftZ))
    {
        EditorSceneSnapshot currentSnapshot = EditorSceneSnapshotBuilder::capture(
            m_sceneContext,
            m_editorState
        );

        EditorSceneSnapshot snapshotToRestore;

        if (m_undoRedoManager.redo(std::move(currentSnapshot), snapshotToRestore))
        {
            restoreHistorySnapshot(snapshotToRestore);
        }
    }

    wasCtrlZ = ctrlZ;
    wasCtrlY = ctrlY;
    wasCtrlShiftZ = ctrlShiftZ;
}