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
    
    // =========================================================
    // --- LÓGICA DO MOUSE (CLIQUE, ARRASTO E SOLTURA) ---
    // =========================================================
    static bool wasLeftMouseDown = false;
    bool isLeftMouseDown = glfwGetMouseButton(m_window->getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    
    bool mouseClicked = isLeftMouseDown && !wasLeftMouseDown;
    bool mouseReleased = !isLeftMouseDown && wasLeftMouseDown; 
    
    wasLeftMouseDown = isLeftMouseDown;

    ImGuiIO& io = ImGui::GetIO();
    
    if (mouseClicked && !io.WantCaptureMouse)
    {
        if (m_editorState.getActiveTool() != EditorToolType::None)
        {
            m_eventBus.emit(EventType::InputMouseClickLeft); // Confirma a ferramenta
        }
        else // Se não tiver nenhuma ferramenta ativa, aí sim o clique serve para selecionar!
        {
        if (!m_transformBridge.handleMouseClick(m_window->getWindow(), m_cameraContext.getCamera(), m_raycaster)) {
            m_selectionController.handleSelection(m_window->getWindow(), m_cameraContext.getCamera(), m_sceneContext.getScene());
        }
    }
}

    if (mouseReleased)
    {
        m_transformBridge.handleMouseRelease();
    }

    // =========================================================
    // --- LÓGICA DO BOTÃO DELETE ---
    // =========================================================
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

    // =========================================================
    // --- ATALHOS DO GIZMO (W, E, R) ---
    // =========================================================
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
    
    // Atualiza o arrasto do mouse
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
        
        // =========================================================
        // ATALHOS DO GIZMO
        // =========================================================
        if (e.type == EventType::InputKeyW || 
            e.type == EventType::InputKeyE || 
            e.type == EventType::InputKeyR ||
            e.type == EventType::InputKeyG ||
            e.type == EventType::InputKeyL) 
        {
            m_transformBridge.handleInputEvent(e.type);
        }
        
        // =========================================================
        // ESCAPE CANCELA TUDO E VOLTA PRO MODO DE OBJETO
        // =========================================================
        else if (e.type == EventType::InputKeyEscape) 
        {
            m_transformBridge.handleInputEvent(e.type);
            m_editorState.setFaceModeActive(false); // Desliga o Face Mode
            m_editorState.clearSelectedFace();      // Limpa seleções de face
            m_toolManager.cancelCurrentTool();      // Cancela extrusão, etc
            m_uiContext.isFaceModeActive = false;   // Atualiza a UI para [OBJECT MODE]
        }

        // =========================================================
        // ATIVAÇÃO DO MODO DE FACE (TECLA 'F' OU BOTÃO NA UI)
        // =========================================================
        else if (e.type == EventType::InputKeyF) 
        {
            // Se já estava ativo, desativa. Se estava desativado, ativa.
            bool currentMode = m_editorState.isFaceModeActive();
            m_editorState.setFaceModeActive(!currentMode);
            m_uiContext.isFaceModeActive = !currentMode; // Atualiza aquele textinho da UI!
            
            // Se desligou o Face Mode, precisamos garantir que as seleções antigas sumam
            if (currentMode) {
                m_editorState.clearSelectedFace();
                m_toolManager.cancelCurrentTool();
            }
        }
        
        // =========================================================
        // FERRAMENTAS DE FACE (EXTRUSÃO, MOVER, ESCALA)
        // =========================================================
        else if (e.type == EventType::InputKeyT || 
                 e.type == EventType::InputKeyM || 
                 e.type == EventType::InputKeyS ||
                 e.type == EventType::InputMouseClickLeft ||
                 e.type == EventType::InputMouseReleaseLeft)
        {
            m_toolManager.handleInputEvent(e.type);
        }
        
        // =========================================================
        // ADIÇÃO DE PRIMITIVAS
        // =========================================================
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
        
        // =========================================================
        // SINCRONIZAÇÃO DO INSPETOR (LÓGICA DO ARRASTO)
        // =========================================================
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
        
        // =========================================================
        // DELEÇÃO DE OBJETO
        // =========================================================
        else if (e.type == EventType::DeleteObject) {
            uint32_t idToDel = static_cast<uint32_t>(e.payloadInt);
            
            if (m_editorState.getSelectedObject() != nullptr && m_editorState.getSelectedObject()->getId() == idToDel) {
                m_editorState.setSelectedObject(nullptr);
                m_uiContext.selectedObjectId = 0;
                
                m_transformBridge.handleInputEvent(EventType::InputKeyEscape);
            }

            m_sceneContext.removeObject(idToDel);
        }
        // ... dentro do setupEventSubscriptions ...

        else if (e.type == EventType::ToolStarted)
        {
            if (m_editorState.isFaceModeActive())
            {
                // Inicia a ferramenta (Extrusão, Mover, etc.) com a câmera atual
                m_faceToolController.startActiveTool(m_window->getWindow(), m_cameraContext.getCamera());
            }
        }
        else if (e.type == EventType::ToolCanceled)
        {
            m_faceToolController.cancelActiveTool();
        }
        else if (e.type == EventType::ToolConfirmed)
        {
            m_faceToolController.confirmActiveTool();
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
}