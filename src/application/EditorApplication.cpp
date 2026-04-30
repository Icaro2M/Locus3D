#include "EditorApplication.h"
#include "../resources/AssetPaths.h"
#include <imgui.h> 

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
    
    // Detecta se houve um clique NOVO (evita atirar raios todo frame)
    static bool wasLeftMouseDown = false;
    bool isLeftMouseDown = glfwGetMouseButton(m_window->getWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    bool mouseClicked = isLeftMouseDown && !wasLeftMouseDown;
    wasLeftMouseDown = isLeftMouseDown;

    ImGuiIO & io = ImGui::GetIO();
    
    // Só tenta selecionar algo se houve um clique e se o clique NÃO foi em cima de uma janela da UI
    if (mouseClicked && !io.WantCaptureMouse)
    {
        if (!m_transformBridge.handleMouseClick(m_window->getWindow(), m_cameraContext.getCamera(), m_raycaster)) {
            m_selectionController.handleSelection(m_window->getWindow(), m_cameraContext.getCamera(), m_sceneContext.getScene());
        }
    }
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
        if (e.type == EventType::AddPrimitive) {
            m_sceneContext.addPrimitive(e.payloadInt);
        }
        else if (e.type == EventType::AddCustomSolid) {
            m_sceneContext.addPrimitive(3);
        }
        else if (e.type == EventType::TransformChanged) {
            // Aplica os números digitados no painel Transform diretamente no objeto real
            SceneObject* selected = m_editorState.getSelectedObject();
            if (selected && selected->getId() == m_uiContext.selectedObjectId) {
                selected->getTransform().setPosition(glm::vec3(m_uiContext.position[0], m_uiContext.position[1], m_uiContext.position[2]));
                selected->getTransform().setRotation(glm::vec3(m_uiContext.rotation[0], m_uiContext.rotation[1], m_uiContext.rotation[2]));
                selected->getTransform().setScale(glm::vec3(m_uiContext.scale[0], m_uiContext.scale[1], m_uiContext.scale[2]));
            }
        }
    });
}

void EditorApplication::syncUI()
{
    // 1. Sincronização via Clique no Inspetor:
    // Se o usuário clicou no nome do objeto no painel, aplicamos essa seleção ao EditorState
    uint32_t stateSelectedId = m_editorState.getSelectedObject() ? m_editorState.getSelectedObject()->getId() : 0;
    
    if (m_uiContext.selectedObjectId != stateSelectedId && m_uiContext.selectedObjectId != 0)
    {
        auto& objects = m_sceneContext.getScene().getObjects();
        for (SceneObject* obj : objects) {
            if (obj->getId() == m_uiContext.selectedObjectId) {
                m_editorState.setSelectedObject(obj);
                stateSelectedId = m_uiContext.selectedObjectId;
                break;
            }
        }
    }

    // 2. Limpa e Atualiza os dados da Cena para a UI
    m_uiContext.sceneObjects.clear();
    auto& objects = m_sceneContext.getScene().getObjects(); 
    
    for (SceneObject* obj : objects)
    {
        SceneObjectInfo info;
        info.id = obj->getId(); 
        info.name = obj->getName(); 
        info.isSelected = (m_editorState.getSelectedObject() == obj);
        m_uiContext.sceneObjects.push_back(info);
    }

    // 3. Sincroniza o Painel Transform
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