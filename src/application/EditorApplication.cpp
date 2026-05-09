#include "EditorApplication.h"
#include "../resources/AssetPaths.h"
#include <imgui.h> 
#include <iostream>

EditorApplication::EditorApplication(WindowManager* window)
    : m_window(window),
      m_shader(AssetPaths::shader("basic/vertex.glsl"), AssetPaths::shader("basic/fragment.glsl")),
      m_toolManager(&m_editorState, &m_eventBus),
      m_selectionController(&m_editorState, &m_eventBus),
      m_transformBridge(&m_editorState, &m_eventBus),
      m_faceToolController(&m_editorState),
      m_renderCoordinator(&m_editorState)
{
    setupEventSubscriptions();
}

void EditorApplication::processInput()
{
    m_inputHandler.process(m_window->getWindow(), &m_eventBus);

    static bool wasLeftMouseDown = false;
    bool isLeftMouseDown = glfwGetMouseButton(m_window->getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    bool mouseClicked = isLeftMouseDown && !wasLeftMouseDown;
    bool mouseReleased = !isLeftMouseDown && wasLeftMouseDown; 
    
    wasLeftMouseDown = isLeftMouseDown;

    ImGuiIO& io = ImGui::GetIO();
    
    if (mouseClicked && !io.WantCaptureMouse)
    {
        if (m_faceToolController.hasRunningTool())
        {
            m_faceToolController.confirmActiveTool();
        }
        else if (m_editorState.getActiveTool() != EditorToolType::None)
        {
            bool selectedFace = m_selectionController.selectFaceUnderMouse(
                m_window->getWindow(),
                m_cameraContext.getCamera()
            );

            if (selectedFace)
            {
                m_faceToolController.startActiveTool(
                    m_window->getWindow(),
                    m_cameraContext.getCamera()
                );
            }
        }
        else
        {
            if (!m_transformBridge.handleMouseClick(
                m_window->getWindow(),
                m_cameraContext.getCamera(),
                m_raycaster
            ))
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
    bool isDeletePressed = glfwGetKey(m_window->getWindow(), GLFW_KEY_DELETE) == GLFW_PRESS;
    
    if (isDeletePressed && !wasDeletePressed && !io.WantTextInput)
    {
        SceneObject* selected = m_editorState.getSelectedObject();
        if (selected != nullptr)
        {
            m_eventBus.emit(EventType::DeleteObject, static_cast<int>(selected->getId()));
        }
    }
    wasDeletePressed = isDeletePressed;

    static bool wasW = false, wasE = false, wasR = false;
    bool isW = glfwGetKey(m_window->getWindow(), GLFW_KEY_W) == GLFW_PRESS;
    bool isE = glfwGetKey(m_window->getWindow(), GLFW_KEY_E) == GLFW_PRESS;
    bool isR = glfwGetKey(m_window->getWindow(), GLFW_KEY_R) == GLFW_PRESS;

    if (isW && !wasW && !io.WantTextInput) {
        m_uiContext.activeTransformMode = TransformMode::Translate;
        m_eventBus.emit(EventType::InputKeyW, 0); 
    }
    if (isE && !wasE && !io.WantTextInput) {
        m_uiContext.activeTransformMode = TransformMode::Rotate;
        m_eventBus.emit(EventType::InputKeyE, 0);
    }
    if (isR && !wasR && !io.WantTextInput) {
        m_uiContext.activeTransformMode = TransformMode::Scale;
        m_eventBus.emit(EventType::InputKeyR, 0);
    }
    
    wasW = isW; 
    wasE = isE; 
    wasR = isR;
}

void EditorApplication::update()
{
    m_cameraContext.update(m_window->getWindow());

    m_transformBridge.handleMouseMove(m_window->getWindow(), m_cameraContext.getCamera(), m_raycaster);
    
    m_faceToolController.update(m_window->getWindow(), m_cameraContext.getCamera());
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
    m_eventBus.subscribe([this](const Event& e) {
        
        if (e.type == EventType::InputKeyW || 
            e.type == EventType::InputKeyE || 
            e.type == EventType::InputKeyR ||
            e.type == EventType::InputKeyG ||
            e.type == EventType::InputKeyL) 
        {
            m_transformBridge.handleInputEvent(e.type);
        }
       
        else if (e.type == EventType::InputKeyEscape)
        {
            if (m_faceToolController.hasRunningTool())
            {
                m_faceToolController.cancelActiveTool();

                m_editorState.setFaceModeActive(true);
                m_uiContext.isFaceModeActive = true;
            }
            else if (m_editorState.getActiveTool() != EditorToolType::None)
            {
                m_toolManager.cancelCurrentTool();

                m_editorState.setFaceModeActive(false);
                m_editorState.clearSelectedFace();
                m_uiContext.isFaceModeActive = false;
            }
            else
            {
                m_transformBridge.handleInputEvent(e.type);

                m_editorState.setFaceModeActive(false);
                m_editorState.clearSelectedFace();
                m_uiContext.isFaceModeActive = false;
            }
        }

        else if (e.type == EventType::InputKeyF)
        {
            /*
                deixar vazio por enquanto
            */
        }
        
        else if (e.type == EventType::InputKeyT ||
            e.type == EventType::InputKeyM ||
            e.type == EventType::InputKeyS)
        {
            m_toolManager.handleInputEvent(e.type);
            m_uiContext.isFaceModeActive = m_editorState.isFaceModeActive();
        }
        
        else if (e.type == EventType::AddPrimitive) {
            m_sceneContext.addPrimitive(e.payloadInt);
        }
        else if (e.type == EventType::AddCustomSolid) {
            m_sceneContext.addCustomSolid(
                m_uiContext.customSolidName,
                m_uiContext.customSolidSides,
                m_uiContext.customSolidBottomRadius,
                m_uiContext.customSolidTopRadius,
                m_uiContext.customSolidHeight
            );
        }
        
        else if (e.type == EventType::TransformChanged) {
            if (!m_transformBridge.getController().isDragging()) {
                SceneObject* selected = m_editorState.getSelectedObject();
                if (selected && selected->getId() == m_uiContext.selectedObjectId) {
                    selected->getTransform().setPosition(glm::vec3(m_uiContext.position[0], m_uiContext.position[1], m_uiContext.position[2]));
                    selected->getTransform().setRotation(glm::vec3(m_uiContext.rotation[0], m_uiContext.rotation[1], m_uiContext.rotation[2]));
                    selected->getTransform().setScale(glm::vec3(m_uiContext.scale[0], m_uiContext.scale[1], m_uiContext.scale[2]));
                }
            }
        }

        else if (e.type == EventType::DeleteObject)
        {
            uint32_t idToDel = static_cast<uint32_t>(e.payloadInt);

            if (m_editorState.getSelectedObject() != nullptr &&
                m_editorState.getSelectedObject()->getId() == idToDel)
            {
                m_editorState.setSelectedObject(nullptr);
                m_uiContext.selectedObjectId = 0;
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
    uint32_t stateSelectedId = m_editorState.getSelectedObject() ? m_editorState.getSelectedObject()->getId() : 0;
    
    if (m_uiContext.selectedObjectId != stateSelectedId && m_uiContext.selectedObjectId != 0)
    {
        auto& objects = m_sceneContext.getScene().getObjects();
        for (SceneObject* obj : objects) {
            if (obj != nullptr && obj->getId() == m_uiContext.selectedObjectId) {
                m_editorState.setSelectedObject(obj);
                stateSelectedId = m_uiContext.selectedObjectId;
                break;
            }
        }
    }

    m_uiContext.sceneObjects.clear();
    auto& objects = m_sceneContext.getScene().getObjects(); 
    
    for (SceneObject* obj : objects)
    {
        if (obj == nullptr) continue; 
        
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
        m_uiContext.position[0] = pos.x; m_uiContext.position[1] = pos.y; m_uiContext.position[2] = pos.z;
        
        glm::vec3 rot = selected->getTransform().getRotation();
        m_uiContext.rotation[0] = rot.x; m_uiContext.rotation[1] = rot.y; m_uiContext.rotation[2] = rot.z;
        
        glm::vec3 scale = selected->getTransform().getScale();
        m_uiContext.scale[0] = scale.x; m_uiContext.scale[1] = scale.y; m_uiContext.scale[2] = scale.z;
    }
    else
    {
        m_uiContext.selectedObjectId = 0;
    }

    if (m_editorState.getSelectedObject() == nullptr)
    {
        clearFaceEditingState();
    }
}

void EditorApplication::clearFaceEditingState()
{
    if (m_faceToolController.hasRunningTool())
    {
        m_faceToolController.cancelActiveTool();
    }

    m_editorState.clearSelectedFace();
    m_editorState.setActiveTool(EditorToolType::None);
    m_editorState.setFaceModeActive(false);

    m_uiContext.isFaceModeActive = false;
}