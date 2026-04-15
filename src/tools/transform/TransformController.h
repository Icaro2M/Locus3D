#pragma once

#include <glm/glm/glm.hpp>

#include "../../scene/SceneObject.h"
#include "../../math/Ray.h"
#include "AxisDragInteraction.h"

enum class TransformMode
{
    None,
    Translate,
    Rotate,
    Scale
};

enum class TransformAxis
{
    None,
    X,
    Y,
    Z
};

enum class TransformSpace
{
    Global,
    Local
};

class TransformController
{
private:
    SceneObject* m_SelectedObject;
    TransformMode m_Mode;
    TransformAxis m_Axis;
    TransformSpace m_Space;

    glm::vec3 m_DragStartPosition;
    AxisDragInteraction m_AxisDrag;

private:
    glm::vec3 getAxisDirectionWorld() const;

public:
    TransformController();

    void setSelectedObject(SceneObject* object);
    SceneObject* getSelectedObject() const;

    void setMode(TransformMode mode);
    TransformMode getMode() const;

    void setAxis(TransformAxis axis);
    TransformAxis getAxis() const;

    void setSpace(TransformSpace space);
    TransformSpace getSpace() const;

    void applyPositiveStep();
    void applyNegativeStep();

    void beginDragFromRay(const Ray& ray);
    void updateDragFromRay(const Ray& ray);
    void endDrag();
    bool isDragging() const;
};