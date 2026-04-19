#pragma once

#include <vector>
#include <string>

struct GLFWwindow;

#include "../../scene/Camera.h"

#include "FaceSelection.h"
#include "FaceGeometry.h"
#include "Raycaster.h"

#include "../transform/AxisDragInteraction.h"

class FaceMoveTool
{
private:
    bool m_Active;
    FaceSelection m_Selection;
    FaceGeometry m_BaseGeometry;

    std::vector<unsigned int> m_CoincidentVertexIndices;

    float m_CurrentDistance;

    std::string m_InputBuffer;
    bool m_UsingNumericInput;
    bool m_HasCommittedNumericValue;

    Raycaster m_Raycaster;
    AxisDragInteraction m_AxisDrag;

private:
    void buildCoincidentVertexSet();

public:
    FaceMoveTool();

    bool start(const FaceSelection& selection, GLFWwindow* window, const Camera& camera);
    void update(GLFWwindow* window, const Camera& camera);
    void onKeyPressed(int key);

    bool confirm();
    void cancel();

    bool isActive() const;
    float getCurrentDistance() const;

    const FaceSelection& getSelection() const;
    const FaceGeometry& getBaseGeometry() const;
    const std::vector<unsigned int>& getCoincidentVertexIndices() const;
};