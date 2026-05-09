#pragma once

#include "../scene/SceneObject.h"
#include "../tools/selection/FaceSelection.h"
#include "../tools/transform/TransformTypes.h"

enum class EditorToolType
{
    None,
    PushPull,
    FaceMove,
    FaceScale
};

class EditorState
{
public:
    EditorState();
    ~EditorState() = default;

    bool isFaceModeActive() const;
    void setFaceModeActive(bool active);

    SceneObject* getSelectedObject() const;
    void setSelectedObject(SceneObject* object);

    FaceSelection& getSelectedFace();
    void clearSelectedFace();

    FaceSelection& getHoveredFace();
    void clearHoveredFace();

    EditorToolType getActiveTool() const;
    void setActiveTool(EditorToolType tool);

    TransformMode getTransformMode() const;
    void setTransformMode(TransformMode mode);

    TransformSpace getTransformSpace() const;
    void setTransformSpace(TransformSpace space);

    TransformAxis getTransformAxis() const;
    void setTransformAxis(TransformAxis axis);

    void clearAllSelections();
    void resetModes();

private:
    bool m_faceModeActive;

    SceneObject* m_selectedObject;

    FaceSelection m_selectedFace;
    FaceSelection m_hoveredFace;

    EditorToolType m_activeTool;

    TransformMode m_transformMode;
    TransformSpace m_transformSpace;
    TransformAxis m_transformAxis;
};