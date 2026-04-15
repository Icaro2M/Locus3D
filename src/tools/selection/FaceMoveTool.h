#pragma once

#include <string>
#include <vector>

#include <glm/glm/glm.hpp>

struct GLFWwindow;

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
    float m_CurrentDistance;
    std::string m_InputBuffer;
    bool m_UsingNumericInput;
    bool m_HasCommittedNumericValue;

    std::vector<float> m_OriginalVertices;
    std::vector<unsigned int> m_OriginalIndices;
    std::vector<unsigned int> m_CoincidentVertexIndices;

    Raycaster m_Raycaster;
    AxisDragInteraction m_AxisDrag;

private:
    bool buildCoincidentVertexSet();
    bool applyMoveToMesh(float distance);
    void clearOperationData();

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
};