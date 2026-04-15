#include "FaceMoveTool.h"

#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>

#include "../../geometry/Mesh.h"
#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"
#include "FaceGeometryBuilder.h"

namespace
{
    constexpr float kPositionEpsilon = 0.0001f;
    constexpr unsigned int kVertexStride = 6;

    glm::vec3 readVertexPosition(const std::vector<float>& vertices, unsigned int vertexIndex)
    {
        const unsigned int base = vertexIndex * kVertexStride;

        return glm::vec3(
            vertices[base + 0],
            vertices[base + 1],
            vertices[base + 2]
        );
    }

    void writeVertexPosition(std::vector<float>& vertices, unsigned int vertexIndex, const glm::vec3& position)
    {
        const unsigned int base = vertexIndex * kVertexStride;

        vertices[base + 0] = position.x;
        vertices[base + 1] = position.y;
        vertices[base + 2] = position.z;
    }

    bool positionsMatch(const glm::vec3& a, const glm::vec3& b, float epsilon)
    {
        glm::vec3 diff = a - b;
        return glm::dot(diff, diff) <= (epsilon * epsilon);
    }

    bool containsVertexIndex(const std::vector<unsigned int>& indices, unsigned int value)
    {
        return std::find(indices.begin(), indices.end(), value) != indices.end();
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

    Mesh& mesh = object->getMesh();
    m_OriginalVertices = mesh.getVertices();
    m_OriginalIndices = mesh.getIndices();

    if (!buildCoincidentVertexSet())
    {
        clearOperationData();
        return false;
    }

    const glm::vec3 localCenter =
        (m_BaseGeometry.getLocalV0() +
            m_BaseGeometry.getLocalV1() +
            m_BaseGeometry.getLocalV2()) / 3.0f;

    glm::vec3 localNormal = m_BaseGeometry.getLocalNormal();

    if (glm::length(localNormal) <= 0.00001f)
    {
        clearOperationData();
        return false;
    }

    localNormal = glm::normalize(localNormal);

    const glm::mat4 modelMatrix = object->getTransform().getModelMatrix();

    const glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(localCenter, 1.0f));
    glm::vec3 worldNormal = glm::mat3(modelMatrix) * localNormal;

    if (glm::length(worldNormal) <= 0.00001f)
    {
        clearOperationData();
        return false;
    }

    worldNormal = glm::normalize(worldNormal);

    const Ray initialRay = m_Raycaster.buildRayFromMouse(window, camera);

    if (!m_AxisDrag.begin(worldCenter, worldNormal, initialRay))
    {
        clearOperationData();
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

bool FaceMoveTool::buildCoincidentVertexSet()
{
    m_CoincidentVertexIndices.clear();

    if (!m_Selection.isValid())
    {
        return false;
    }

    SceneObject* object = m_Selection.getObject();
    if (object == nullptr)
    {
        return false;
    }

    Mesh& mesh = object->getMesh();

    unsigned int i0 = 0;
    unsigned int i1 = 0;
    unsigned int i2 = 0;

    if (!mesh.getTriangleVertexIndices(m_Selection.getFaceIndex(), i0, i1, i2))
    {
        return false;
    }

    const std::vector<unsigned int> faceVertexIndices = { i0, i1, i2 };
    const unsigned int totalVertexCount =
        static_cast<unsigned int>(m_OriginalVertices.size() / kVertexStride);

    for (unsigned int faceVertexIndex : faceVertexIndices)
    {
        const glm::vec3 targetPosition =
            readVertexPosition(m_OriginalVertices, faceVertexIndex);

        for (unsigned int candidateIndex = 0; candidateIndex < totalVertexCount; ++candidateIndex)
        {
            const glm::vec3 candidatePosition =
                readVertexPosition(m_OriginalVertices, candidateIndex);

            if (!positionsMatch(targetPosition, candidatePosition, kPositionEpsilon))
            {
                continue;
            }

            if (containsVertexIndex(m_CoincidentVertexIndices, candidateIndex))
            {
                continue;
            }

            m_CoincidentVertexIndices.push_back(candidateIndex);
        }
    }

    return !m_CoincidentVertexIndices.empty();
}

bool FaceMoveTool::applyMoveToMesh(float distance)
{
    if (!m_Selection.isValid())
    {
        return false;
    }

    SceneObject* object = m_Selection.getObject();
    if (object == nullptr)
    {
        return false;
    }

    if (m_OriginalVertices.empty() || m_OriginalIndices.empty())
    {
        return false;
    }

    if (m_CoincidentVertexIndices.empty())
    {
        return false;
    }

    glm::vec3 localNormal = m_BaseGeometry.getLocalNormal();

    if (glm::length(localNormal) <= 0.00001f)
    {
        return false;
    }

    localNormal = glm::normalize(localNormal);
    const glm::vec3 offset = localNormal * distance;

    std::vector<float> movedVertices = m_OriginalVertices;

    for (unsigned int vertexIndex : m_CoincidentVertexIndices)
    {
        const glm::vec3 originalPosition =
            readVertexPosition(m_OriginalVertices, vertexIndex);

        const glm::vec3 movedPosition = originalPosition + offset;
        writeVertexPosition(movedVertices, vertexIndex, movedPosition);
    }

    Mesh& mesh = object->getMesh();
    mesh.updateGeometry(movedVertices, m_OriginalIndices);

    return true;
}

bool FaceMoveTool::confirm()
{
    if (!m_Active || !m_Selection.isValid())
    {
        return false;
    }

    const bool success = applyMoveToMesh(m_CurrentDistance);
    clearOperationData();

    return success;
}

void FaceMoveTool::cancel()
{
    if (!m_Active)
    {
        return;
    }

    clearOperationData();
}

void FaceMoveTool::clearOperationData()
{
    m_Active = false;
    m_Selection.clear();
    m_BaseGeometry.clear();
    m_CurrentDistance = 0.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_OriginalVertices.clear();
    m_OriginalIndices.clear();
    m_CoincidentVertexIndices.clear();
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

const FaceSelection& FaceMoveTool::getSelection() const
{
    return m_Selection;
}

const FaceGeometry& FaceMoveTool::getBaseGeometry() const
{
    return m_BaseGeometry;
}