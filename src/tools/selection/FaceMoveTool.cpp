#include "FaceMoveTool.h"

#include <iostream>
#include <algorithm>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "../../geometry/Mesh.h"
#include "../../scene/SceneObject.h"
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
    m_StartMouseY(0.0),
    m_CurrentDistance(0.0f),
    m_Sensitivity(0.01f),
    m_InputBuffer(""),
    m_UsingNumericInput(false),
    m_HasCommittedNumericValue(false)
{
}

bool FaceMoveTool::start(const FaceSelection& selection, GLFWwindow* window)
{
    if (!selection.isValid())
    {
        std::cout << "[FACE MOVE] Nenhuma face valida selecionada\n";
        return false;
    }

    SceneObject* object = selection.getObject();
    if (object == nullptr)
    {
        std::cout << "[FACE MOVE] Objeto da selecao e nulo\n";
        return false;
    }

    m_Selection = selection;

    FaceGeometryBuilder builder;
    m_BaseGeometry = builder.build(m_Selection);

    if (!m_BaseGeometry.isValid())
    {
        std::cout << "[FACE MOVE] Falha ao construir geometria da face\n";
        m_Selection.clear();
        return false;
    }

    Mesh& mesh = object->getMesh();
    m_OriginalVertices = mesh.getVertices();
    m_OriginalIndices = mesh.getIndices();

    if (!buildCoincidentVertexSet())
    {
        std::cout << "[FACE MOVE] Falha ao mapear vertices coincidentes\n";
        clearOperationData();
        return false;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    m_StartMouseY = mouseY;
    m_CurrentDistance = 0.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_Active = true;

    std::cout << "[FACE MOVE] Ferramenta iniciada\n";
    return true;
}

void FaceMoveTool::update(GLFWwindow* window)
{
    if (!m_Active)
    {
        return;
    }

    if (m_UsingNumericInput || m_HasCommittedNumericValue)
    {
        return;
    }

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    double deltaY = m_StartMouseY - mouseY;
    m_CurrentDistance = static_cast<float>(deltaY) * m_Sensitivity;
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
        std::cout << "[FACE MOVE INPUT] " << m_InputBuffer << "\n";
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
            std::cout << "[FACE MOVE INPUT] " << m_InputBuffer << "\n";
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
            std::cout << "[FACE MOVE INPUT] " << m_InputBuffer << "\n";
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

            std::cout << "[FACE MOVE INPUT] " << m_InputBuffer << "\n";
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
                std::cout << "[FACE MOVE INPUT] Distancia definida: " << m_CurrentDistance << "\n";
            }
            catch (...)
            {
                std::cout << "[FACE MOVE INPUT] Valor invalido\n";
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
        std::cout << "[FACE MOVE] Nenhuma operacao ativa para confirmar\n";
        return false;
    }

    const bool success = applyMoveToMesh(m_CurrentDistance);

    if (success)
    {
        std::cout << "[FACE MOVE] Face movida | distancia = " << m_CurrentDistance << "\n";
    }
    else
    {
        std::cout << "[FACE MOVE] Falha ao mover face\n";
    }

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
    std::cout << "[FACE MOVE] Cancelado\n";
}

void FaceMoveTool::clearOperationData()
{
    m_Active = false;
    m_Selection.clear();
    m_BaseGeometry.clear();
    m_StartMouseY = 0.0;
    m_CurrentDistance = 0.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_OriginalVertices.clear();
    m_OriginalIndices.clear();
    m_CoincidentVertexIndices.clear();
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