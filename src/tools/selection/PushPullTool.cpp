#include "PushPullTool.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>

#include "FaceGeometryBuilder.h"
#include "FaceExtruder.h"
#include "../../scene/SceneObject.h"

PushPullTool::PushPullTool()
    : m_Active(false),
    m_CurrentDistance(0.0f),
    m_InputBuffer(""),
    m_UsingNumericInput(false),
    m_HasCommittedNumericValue(false)
{
}

bool PushPullTool::start(const FaceSelection& selection, GLFWwindow* window, const Camera& camera)
{
    if (!selection.isValid())
    {
        return false;
    }

    SceneObject* object = selection.getObject();
    if (object == nullptr)
    {
        return false;
    }

    m_Selection = selection;

    FaceGeometryBuilder builder;
    m_BaseGeometry = builder.build(m_Selection);

    if (!m_BaseGeometry.isValid())
    {
        m_Selection.clear();
        return false;
    }

    const glm::vec3 localCenter =
        (m_BaseGeometry.getLocalV0() +
            m_BaseGeometry.getLocalV1() +
            m_BaseGeometry.getLocalV2()) / 3.0f;

    glm::vec3 localNormal = m_BaseGeometry.getLocalNormal();

    if (glm::length(localNormal) <= 0.00001f)
    {
        m_Selection.clear();
        m_BaseGeometry.clear();
        return false;
    }

    localNormal = glm::normalize(localNormal);

    const glm::mat4 modelMatrix = object->getTransform().getModelMatrix();

    const glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(localCenter, 1.0f));
    glm::vec3 worldNormal = glm::mat3(modelMatrix) * localNormal;

    if (glm::length(worldNormal) <= 0.00001f)
    {
        m_Selection.clear();
        m_BaseGeometry.clear();
        return false;
    }

    worldNormal = glm::normalize(worldNormal);

    const Ray initialRay = m_Raycaster.buildRayFromMouse(window, camera);

    if (!m_AxisDrag.begin(worldCenter, worldNormal, initialRay))
    {
        m_Selection.clear();
        m_BaseGeometry.clear();
        return false;
    }

    m_CurrentDistance = 0.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_Active = true;

    return true;
}

void PushPullTool::update(GLFWwindow* window, const Camera& camera)
{
    if (!m_Active)
    {
        return;
    }

    if (m_UsingNumericInput || m_HasCommittedNumericValue)
    {
        return;
    }

    const Ray currentRay = m_Raycaster.buildRayFromMouse(window, camera);
    m_AxisDrag.update(currentRay);
    m_CurrentDistance = m_AxisDrag.getCurrentDelta();
}

void PushPullTool::onKeyPressed(int key)
{
    if (!m_Active)
    {
        return;
    }

    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
    {
        m_InputBuffer += static_cast<char>('0' + (key - GLFW_KEY_0));
        m_UsingNumericInput = true;
        m_HasCommittedNumericValue = false;
        return;
    }

    if (key == GLFW_KEY_PERIOD || key == GLFW_KEY_KP_DECIMAL)
    {
        if (m_InputBuffer.find('.') == std::string::npos)
        {
            if (m_InputBuffer.empty() || m_InputBuffer == "-")
            {
                m_InputBuffer += "0";
            }

            m_InputBuffer += '.';
            m_UsingNumericInput = true;
            m_HasCommittedNumericValue = false;
        }

        return;
    }

    if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT)
    {
        if (m_InputBuffer.empty())
        {
            m_InputBuffer = "-";
            m_UsingNumericInput = true;
            m_HasCommittedNumericValue = false;
        }

        return;
    }

    if (key == GLFW_KEY_BACKSPACE)
    {
        if (!m_InputBuffer.empty())
        {
            m_InputBuffer.pop_back();

            if (m_InputBuffer.empty())
            {
                m_UsingNumericInput = false;
            }
        }

        return;
    }

    if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
    {
        if (!m_InputBuffer.empty() &&
            m_InputBuffer != "-" &&
            m_InputBuffer != "." &&
            m_InputBuffer != "-.")
        {
            try
            {
                m_CurrentDistance = std::stof(m_InputBuffer);
                m_HasCommittedNumericValue = true;
            }
            catch (...)
            {
            }
        }

        m_InputBuffer.clear();
        m_UsingNumericInput = false;
        return;
    }
}

bool PushPullTool::confirm()
{
    if (!m_Active || !m_Selection.isValid())
    {
        return false;
    }

    FaceExtruder extruder;
    const bool success = extruder.extrude(m_Selection, m_CurrentDistance);

    m_Active = false;
    m_Selection.clear();
    m_BaseGeometry.clear();
    m_CurrentDistance = 0.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_AxisDrag.reset();

    return success;
}

void PushPullTool::cancel()
{
    if (!m_Active)
    {
        return;
    }

    m_Active = false;
    m_Selection.clear();
    m_BaseGeometry.clear();
    m_CurrentDistance = 0.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_AxisDrag.reset();
}

bool PushPullTool::isActive() const
{
    return m_Active;
}

float PushPullTool::getCurrentDistance() const
{
    return m_CurrentDistance;
}

const FaceSelection& PushPullTool::getSelection() const
{
    return m_Selection;
}

const FaceGeometry& PushPullTool::getBaseGeometry() const
{
    return m_BaseGeometry;
}