#include "FaceMoveTool.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>

#include <glm/glm/glm.hpp>

#include "../../geometry/Mesh.h"
#include "../../scene/SceneObject.h"
#include "FaceGeometryBuilder.h"

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

    m_Selection = selection;

    FaceGeometryBuilder builder;
    m_BaseGeometry = builder.build(m_Selection);

    if (!m_BaseGeometry.isValid())
    {
        std::cout << "[FACE MOVE] Falha ao construir geometria da face\n";
        m_Selection.clear();
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

                std::cout << "[FACE MOVE INPUT] Distancia definida: "
                    << m_CurrentDistance << "\n";
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

    Mesh& mesh = object->getMesh();

    std::vector<float> vertices = mesh.getVertices();
    std::vector<unsigned int> indices = mesh.getIndices();

    const int faceIndex = m_Selection.getFaceIndex();

    unsigned int i0 = 0;
    unsigned int i1 = 0;
    unsigned int i2 = 0;

    if (!mesh.getTriangleVertexIndices(faceIndex, i0, i1, i2))
    {
        return false;
    }

    glm::vec3 localNormal = m_BaseGeometry.getLocalNormal();

    if (glm::length(localNormal) <= 0.00001f)
    {
        return false;
    }

    localNormal = glm::normalize(localNormal);
    glm::vec3 offset = localNormal * distance;

    auto applyOffsetToVertex = [&](unsigned int vertexIndex)
        {
            const unsigned int base = vertexIndex * 6;

            if (base + 2 >= vertices.size())
            {
                return;
            }

            vertices[base + 0] += offset.x;
            vertices[base + 1] += offset.y;
            vertices[base + 2] += offset.z;
        };

    applyOffsetToVertex(i0);
    applyOffsetToVertex(i1);
    applyOffsetToVertex(i2);

    mesh.updateGeometry(vertices, indices);
    return true;
}

bool FaceMoveTool::confirm()
{
    if (!m_Active || !m_Selection.isValid())
    {
        std::cout << "[FACE MOVE] Nenhuma operacao ativa para confirmar\n";
        return false;
    }

    bool success = applyMoveToMesh(m_CurrentDistance);

    if (success)
    {
        std::cout << "[FACE MOVE] Face movida | distancia = "
            << m_CurrentDistance << "\n";
    }
    else
    {
        std::cout << "[FACE MOVE] Falha ao mover face\n";
    }

    m_Active = false;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;

    return success;
}

void FaceMoveTool::cancel()
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

    std::cout << "[FACE MOVE] Cancelado\n";
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