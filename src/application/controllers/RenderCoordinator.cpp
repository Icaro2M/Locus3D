#include "RenderCoordinator.h"

RenderCoordinator::RenderCoordinator(EditorState* state)
    : m_state(state)
{
}

void RenderCoordinator::render(Scene& scene, Camera& camera, Shader& shader, 
                               FaceToolController& faceTools, TransformController& transformController)
{
    m_renderer.clear();
    m_renderer.renderScene(scene, camera, shader);

    m_gridRenderer.render(camera);
    m_axisRenderer.render(camera);

    m_pushPullPreview.render(faceTools.getPushPullTool(), camera);
    m_faceMovePreview.render(faceTools.getFaceMoveTool(), camera);
    m_faceScalePreview.render(faceTools.getFaceScaleTool(), camera);

    SceneObject* selectedObject = m_state->getSelectedObject();

    if (selectedObject != nullptr)
    {
        m_selectionOutlineRenderer.render(*selectedObject, camera);

        EditorToolType activeTool = m_state->getActiveTool();
        
        if (activeTool != EditorToolType::PushPull &&
            activeTool != EditorToolType::FaceMove &&
            activeTool != EditorToolType::FaceScale)
        {
            TransformMode mode = m_state->getTransformMode();
            TransformAxis axis = transformController.getAxis();
            TransformSpace space = transformController.getSpace();

            if (mode == TransformMode::Translate)
            {
                m_translateGizmoRenderer.render(*selectedObject, camera, axis, space);
            }
            else if (mode == TransformMode::Scale)
            {
                m_scaleGizmoRenderer.render(*selectedObject, camera, axis, space);
            }
            else if (mode == TransformMode::Rotate)
            {
                m_rotateGizmoRenderer.render(*selectedObject, camera, axis, space);
            }
        }

        FaceSelection& selectedFace = m_state->getSelectedFace();
        
        if (selectedFace.isValid())
        {
            m_faceHighlightRenderer.render(*selectedFace.getObject(), selectedFace.getFaceIndex(), camera);
        }
    }
}