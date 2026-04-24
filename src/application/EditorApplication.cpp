#include "EditorApplication.h"
#include "../resources/AssetPaths.h"

EditorApplication::EditorApplication(WindowManager* window)
    : m_window(window),
      m_shader(AssetPaths::shader("basic/vertex.glsl"), AssetPaths::shader("basic/fragment.glsl")),
      m_toolManager(&m_editorState, &m_eventBus),
      m_selectionController(&m_editorState, &m_eventBus),
      m_transformBridge(&m_editorState, &m_eventBus),
      m_faceToolController(&m_editorState),
      m_renderCoordinator(&m_editorState)
{
}

void EditorApplication::processInput()
{
    m_inputHandler.process(m_window->getWindow(), &m_eventBus);
    
    if (m_transformBridge.handleMouseClick(m_window->getWindow(), m_cameraContext.getCamera(), m_raycaster)) {
        // Clique consumido pelo Gizmo
    } else {
        m_selectionController.handleSelection(m_window->getWindow(), m_cameraContext.getCamera(), m_sceneContext.getScene());
    }
}

void EditorApplication::update()
{
    m_cameraContext.update(m_window->getWindow());
    m_transformBridge.handleMouseMove(m_window->getWindow(), m_cameraContext.getCamera(), m_raycaster);
    m_faceToolController.update(m_window->getWindow(), m_cameraContext.getCamera());
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