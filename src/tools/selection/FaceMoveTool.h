#pragma once

#include <string>

struct GLFWwindow;

#include "FaceSelection.h"
#include "FaceGeometry.h"

class FaceMoveTool
{
private:
    bool m_Active;
    FaceSelection m_Selection;
    FaceGeometry m_BaseGeometry;

    double m_StartMouseY;
    float m_CurrentDistance;
    float m_Sensitivity;

    std::string m_InputBuffer;
    bool m_UsingNumericInput;
    bool m_HasCommittedNumericValue;

private:
    bool applyMoveToMesh(float distance);

public:
    FaceMoveTool();

    bool start(const FaceSelection& selection, GLFWwindow* window);
    void update(GLFWwindow* window);

    void onKeyPressed(int key);

    bool confirm();
    void cancel();

    bool isActive() const;
    float getCurrentDistance() const;

    const FaceSelection& getSelection() const;
    const FaceGeometry& getBaseGeometry() const;
};