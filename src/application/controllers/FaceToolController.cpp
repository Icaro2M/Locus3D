#include "FaceToolController.h"

FaceToolController::FaceToolController(EditorState* state)
    : m_state(state)
{
}

bool FaceToolController::startActiveTool(GLFWwindow* window, Camera& camera)
{
    if (!m_state->getSelectedFace().isValid())
    {
        return false;
    }

    EditorToolType activeTool = m_state->getActiveTool();

    if (activeTool == EditorToolType::PushPull)
    {
        return m_pushPullTool.start(m_state->getSelectedFace(), window, camera);
    }
    else if (activeTool == EditorToolType::FaceMove)
    {
        return m_faceMoveTool.start(m_state->getSelectedFace(), window, camera);
    }
    else if (activeTool == EditorToolType::FaceScale)
    {
        return m_faceScaleTool.start(m_state->getSelectedFace(), window, camera);
    }

    return false;
}

void FaceToolController::update(GLFWwindow* window, Camera& camera)
{
    EditorToolType activeTool = m_state->getActiveTool();

    if (activeTool == EditorToolType::PushPull)
    {
        m_pushPullTool.update(window, camera);
    }
    else if (activeTool == EditorToolType::FaceMove)
    {
        m_faceMoveTool.update(window, camera);
    }
    else if (activeTool == EditorToolType::FaceScale)
    {
        m_faceScaleTool.update(window, camera);
    }
}

bool FaceToolController::confirmActiveTool()
{
    EditorToolType activeTool = m_state->getActiveTool();

    bool success = false;

    if (activeTool == EditorToolType::PushPull)
    {
        success = m_pushPullTool.confirm();
    }
    else if (activeTool == EditorToolType::FaceMove)
    {
        success = m_faceMoveTool.confirm();
    }
    else if (activeTool == EditorToolType::FaceScale)
    {
        success = m_faceScaleTool.confirm();
    }

    if (success)
    {
        m_state->clearSelectedFace();
    }

    return success;
}

void FaceToolController::cancelActiveTool()
{
    EditorToolType activeTool = m_state->getActiveTool();

    if (activeTool == EditorToolType::PushPull)
    {
        m_pushPullTool.cancel();
    }
    else if (activeTool == EditorToolType::FaceMove)
    {
        m_faceMoveTool.cancel();
    }
    else if (activeTool == EditorToolType::FaceScale)
    {
        m_faceScaleTool.cancel();
    }

    m_state->clearSelectedFace();
}

bool FaceToolController::hasRunningTool() const
{
    return m_pushPullTool.isActive() ||
        m_faceMoveTool.isActive() ||
        m_faceScaleTool.isActive();
}

void FaceToolController::handleKeyPress(int key)
{
    EditorToolType activeTool = m_state->getActiveTool();

    if (activeTool == EditorToolType::PushPull)
    {
        m_pushPullTool.onKeyPressed(key);
    }
    else if (activeTool == EditorToolType::FaceMove)
    {
        m_faceMoveTool.onKeyPressed(key);
    }
    else if (activeTool == EditorToolType::FaceScale)
    {
        m_faceScaleTool.onKeyPressed(key);
    }
}

PushPullTool& FaceToolController::getPushPullTool()
{
    return m_pushPullTool;
}

FaceMoveTool& FaceToolController::getFaceMoveTool()
{
    return m_faceMoveTool;
}

FaceScaleTool& FaceToolController::getFaceScaleTool()
{
    return m_faceScaleTool;
}