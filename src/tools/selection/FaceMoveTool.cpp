#include "FaceMoveTool.h"

#include <GLFW/glfw3.h>

#include <glm/glm/glm.hpp>

#include "../../geometry/Mesh.h"
#include "../../scene/SceneObject.h"

#include "FaceGeometryBuilder.h"
#include "../../geometry/LogicalFace.h"

namespace
{
    bool containsIndex(
        const std::vector<unsigned int>& values,
        unsigned int target
    )
    {
        for (unsigned int value : values)
        {
            if (value == target)
            {
                return true;
            }
        }

        return false;
    }

    bool isSamePosition(
        const glm::vec3& a,
        const glm::vec3& b,
        float epsilon = 0.0001f
    )
    {
        return glm::length(a - b) <= epsilon;
    }
}

FaceMoveTool::FaceMoveTool()
    : m_Active(false),
    m_CurrentDistance(0.0f),
    m_InputBuffer(""),
    m_UsingNumericInput(false),
    m_HasCommittedNumericValue(false)
{
}

bool FaceMoveTool::start(const FaceSelection& selection, GLFWwindow* window, const Camera& camera)
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

    glm::vec3 localCenter = m_BaseGeometry.getLocalCenter();
    glm::vec3 localNormal = m_BaseGeometry.getLocalNormal();

    if (glm::length(localNormal) <= 0.00001f)
    {
        m_Selection.clear();
        m_BaseGeometry.clear();
        return false;
    }

    localNormal = glm::normalize(localNormal);

    const glm::mat4 modelMatrix = object->getTransform().getModelMatrix();

    const glm::vec3 worldCenter =
        glm::vec3(modelMatrix * glm::vec4(localCenter, 1.0f));

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

    buildCoincidentVertexSet();

    if (m_CoincidentVertexIndices.empty())
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

void FaceMoveTool::update(GLFWwindow* window, const Camera& camera)
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

void FaceMoveTool::onKeyPressed(int key)
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

bool FaceMoveTool::confirm()
{
    if (!m_Active || !m_Selection.isValid())
    {
        return false;
    }

    SceneObject* object = m_Selection.getObject();
    if (object == nullptr)
    {
        cancel();
        return false;
    }

    Mesh& mesh = object->getMesh();

    std::vector<float> vertices = mesh.getVertices();
    std::vector<LogicalFace> logicalFaces = mesh.getLogicalFaces();

    glm::vec3 normal = m_BaseGeometry.getLocalNormal();
    if (glm::length(normal) <= 0.00001f)
    {
        cancel();
        return false;
    }

    glm::vec3 moveDelta = glm::normalize(normal) * m_CurrentDistance;

    for (unsigned int vertexIndex : m_CoincidentVertexIndices)
    {
        unsigned int base = vertexIndex * 6;

        if (base + 2 >= vertices.size())
        {
            continue;
        }

        vertices[base + 0] += moveDelta.x;
        vertices[base + 1] += moveDelta.y;
        vertices[base + 2] += moveDelta.z;
    }

    mesh.updateGeometry(vertices, mesh.getIndices());
    mesh.setLogicalFaces(logicalFaces);

    cancel();
    return true;
}

void FaceMoveTool::cancel()
{
    m_Active = false;
    m_Selection.clear();
    m_BaseGeometry.clear();
    m_CoincidentVertexIndices.clear();
    m_CurrentDistance = 0.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_AxisDrag.reset();
}

bool FaceMoveTool::isActive() const
{
    return m_Active;
}

float FaceMoveTool::getCurrentDistance() const
{
    return m_CurrentDistance;
}

void FaceMoveTool::setCurrentDistanceFromNumericInput(float distance)
{
    if (!m_Active)
    {
        return;
    }

    m_CurrentDistance = distance;
    m_UsingNumericInput = true;
    m_HasCommittedNumericValue = true;
}

const FaceSelection& FaceMoveTool::getSelection() const
{
    return m_Selection;
}

const FaceGeometry& FaceMoveTool::getBaseGeometry() const
{
    return m_BaseGeometry;
}

const std::vector<unsigned int>& FaceMoveTool::getCoincidentVertexIndices() const
{
    return m_CoincidentVertexIndices;
}

void FaceMoveTool::buildCoincidentVertexSet()
{
    m_CoincidentVertexIndices.clear();

    if (!m_BaseGeometry.isValid())
    {
        return;
    }

    SceneObject* object = m_BaseGeometry.getObject();
    if (object == nullptr)
    {
        return;
    }

    const Mesh& mesh = object->getMesh();
    const std::vector<float>& vertices = mesh.getVertices();
    const std::vector<unsigned int>& triangleIndices = m_BaseGeometry.getTriangleIndices();

    if (triangleIndices.empty())
    {
        return;
    }

    std::vector<glm::vec3> facePositions;

    for (unsigned int triangleIndex : triangleIndices)
    {
        unsigned int i0 = 0;
        unsigned int i1 = 0;
        unsigned int i2 = 0;

        if (!mesh.getTriangleVertexIndices(triangleIndex, i0, i1, i2))
        {
            continue;
        }

        facePositions.push_back(mesh.getVertexPosition(i0));
        facePositions.push_back(mesh.getVertexPosition(i1));
        facePositions.push_back(mesh.getVertexPosition(i2));
    }

    const unsigned int vertexCount = static_cast<unsigned int>(vertices.size() / 6);

    for (unsigned int candidateIndex = 0; candidateIndex < vertexCount; ++candidateIndex)
    {
        glm::vec3 candidatePosition = mesh.getVertexPosition(candidateIndex);

        for (const glm::vec3& facePosition : facePositions)
        {
            if (isSamePosition(candidatePosition, facePosition))
            {
                if (!containsIndex(m_CoincidentVertexIndices, candidateIndex))
                {
                    m_CoincidentVertexIndices.push_back(candidateIndex);
                }

                break;
            }
        }
    }
}
