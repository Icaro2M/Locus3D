#include "EditorState.h"

EditorState::EditorState()
    : m_faceModeActive(false),
    m_selectedObject(nullptr),
    m_activeTool(EditorToolType::None),
    m_transformMode(TransformMode::None),
    m_transformSpace(TransformSpace::Global),
    m_transformAxis(TransformAxis::None)
{
}

bool EditorState::isFaceModeActive() const
{
    return m_faceModeActive;
}

void EditorState::setFaceModeActive(bool active)
{
    m_faceModeActive = active;

    if (!m_faceModeActive)
    {
        m_selectedFace.clear();
        m_hoveredFace.clear();
    }
}

SceneObject* EditorState::getSelectedObject() const
{
    return m_selectedObject;
}

void EditorState::setSelectedObject(SceneObject* object)
{
    m_selectedObject = object;
    m_selectedFace.clear();
    m_hoveredFace.clear();
}

FaceSelection& EditorState::getSelectedFace()
{
    return m_selectedFace;
}

void EditorState::clearSelectedFace()
{
    m_selectedFace.clear();
}

FaceSelection& EditorState::getHoveredFace()
{
    return m_hoveredFace;
}

void EditorState::clearHoveredFace()
{
    m_hoveredFace.clear();
}

EditorToolType EditorState::getActiveTool() const
{
    return m_activeTool;
}

void EditorState::setActiveTool(EditorToolType tool)
{
    m_activeTool = tool;

    if (m_activeTool == EditorToolType::None)
    {
        m_hoveredFace.clear();
    }
}

TransformMode EditorState::getTransformMode() const
{
    return m_transformMode;
}

void EditorState::setTransformMode(TransformMode mode)
{
    m_transformMode = mode;
}

TransformSpace EditorState::getTransformSpace() const
{
    return m_transformSpace;
}

void EditorState::setTransformSpace(TransformSpace space)
{
    m_transformSpace = space;
}

TransformAxis EditorState::getTransformAxis() const
{
    return m_transformAxis;
}

void EditorState::setTransformAxis(TransformAxis axis)
{
    m_transformAxis = axis;
}

void EditorState::clearAllSelections()
{
    m_selectedObject = nullptr;
    m_selectedFace.clear();
    m_hoveredFace.clear();
}

void EditorState::resetModes()
{
    m_faceModeActive = false;
    m_activeTool = EditorToolType::None;
    m_transformMode = TransformMode::None;
    m_transformSpace = TransformSpace::Global;
    m_transformAxis = TransformAxis::None;

    m_selectedFace.clear();
    m_hoveredFace.clear();
}