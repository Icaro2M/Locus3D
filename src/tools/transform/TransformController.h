#pragma once

#include <glm/glm/glm.hpp>

#include "../../scene/SceneObject.h"
#include "../../math/Ray.h"
#include "AxisDragInteraction.h"
#include "RotateDragInteraction.h"
#include "TransformTypes.h"

class TransformController
{
private:
    SceneObject* m_SelectedObject;
    TransformMode m_Mode;
    TransformAxis m_Axis;
    TransformSpace m_Space;

    glm::vec3 m_DragStartPosition;
    glm::vec3 m_DragStartScale;
    glm::vec3 m_DragStartRotation;

    AxisDragInteraction m_AxisDrag;
    RotateDragInteraction m_RotateDrag;

private:
    glm::vec3 getAxisDirectionWorld() const;
    glm::vec3 buildAxisVector(TransformAxis axis) const;

public:
    TransformController();

    void setSelectedObject(SceneObject* object);
    SceneObject* getSelectedObject() const;

    void clearSelection();

    void setMode(TransformMode mode);
    TransformMode getMode() const;

    void setAxis(TransformAxis axis);
    TransformAxis getAxis() const;

    void clearAxis();

    void setSpace(TransformSpace space);
    TransformSpace getSpace() const;

    bool hasSelection() const;
    bool hasActiveMode() const;

    bool beginDragFromRay(const Ray& ray);
    void updateDragFromRay(const Ray& ray);
    void endDrag();

    bool isDragging() const;
};