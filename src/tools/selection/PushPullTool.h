#pragma once

#include <string>

struct GLFWwindow;

#include "../../scene/Camera.h"

#include "FaceSelection.h"
#include "FaceGeometry.h"
#include "Raycaster.h"

#include "../transform/AxisDragInteraction.h"

class PushPullTool
{
private:
    bool m_Active;
    FaceSelection m_Selection;
    FaceGeometry m_BaseGeometry;
    float m_CurrentDistance;

    std::string m_InputBuffer;
    bool m_UsingNumericInput;
    bool m_HasCommittedNumericValue;

    Raycaster m_Raycaster;
    AxisDragInteraction m_AxisDrag;

public:
    PushPullTool();

    bool start(const FaceSelection& selection, GLFWwindow* window, const Camera& camera);
    void update(GLFWwindow* window, const Camera& camera);
    void onKeyPressed(int key);

    bool confirm();
    void cancel();

    bool isActive() const;
    float getCurrentDistance() const;
    const FaceSelection& getSelection() const;
    const FaceGeometry& getBaseGeometry() const;
};