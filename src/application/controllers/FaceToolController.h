#pragma once

#include "../EditorState.h"
#include "../../scene/Camera.h"
#include "../../tools/selection/PushPullTool.h"
#include "../../tools/selection/FaceMoveTool.h"
#include "../../tools/selection/FaceScaleTool.h"

#include <GLFW/glfw3.h>
#include <string>

class FaceToolController
{
public:
    FaceToolController(EditorState* state);
    ~FaceToolController() = default;

    bool startActiveTool(GLFWwindow* window, Camera& camera);
    void update(GLFWwindow* window, Camera& camera);

    bool confirmActiveTool();
    void cancelActiveTool();

    bool hasRunningTool() const;

    void handleKeyPress(int key);

    bool getActiveNumericInput(float& value) const;
    bool applyActiveNumericInput(const std::string& text);

    PushPullTool& getPushPullTool();
    FaceMoveTool& getFaceMoveTool();
    FaceScaleTool& getFaceScaleTool();

private:
    EditorState* m_state;

    PushPullTool m_pushPullTool;
    FaceMoveTool m_faceMoveTool;
    FaceScaleTool m_faceScaleTool;
};
