#pragma once

#include "../EditorState.h"
#include "../../scene/Scene.h"
#include "../../scene/Camera.h"
#include "../../graphics/Renderer.h"
#include "../../graphics/Shader.h"
#include "../../tools/AxisRenderer.h"
#include "../../tools/GridRenderer.h"
#include "../../tools/selection/SelectionOutlineRenderer.h"
#include "../../tools/selection/FaceHighlightRenderer.h"
#include "../../tools/selection/PushPullPreviewRenderer.h"
#include "../../tools/selection/FaceMovePreviewRenderer.h"
#include "../../tools/selection/FaceScalePreviewRenderer.h"
#include "../../tools/transform/TranslateGizmoRenderer.h"
#include "../../tools/transform/RotateGizmoRenderer.h"
#include "../../tools/transform/ScaleGizmoRenderer.h"
#include "FaceToolController.h"
#include "../../tools/transform/TransformController.h"

class RenderCoordinator {
public:
    RenderCoordinator(EditorState* state);
    ~RenderCoordinator() = default;

    void render(Scene& scene, Camera& camera, Shader& shader, 
                FaceToolController& faceTools, TransformController& transformController);

private:
    EditorState* m_state;

    Renderer m_renderer;
    AxisRenderer m_axisRenderer;
    GridRenderer m_gridRenderer;

    SelectionOutlineRenderer m_selectionOutlineRenderer;
    FaceHighlightRenderer m_faceHighlightRenderer;

    PushPullPreviewRenderer m_pushPullPreview;
    FaceMovePreviewRenderer m_faceMovePreview;
    FaceScalePreviewRenderer m_faceScalePreview;

    TranslateGizmoRenderer m_translateGizmoRenderer;
    RotateGizmoRenderer m_rotateGizmoRenderer;
    ScaleGizmoRenderer m_scaleGizmoRenderer;
};