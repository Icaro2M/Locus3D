#include "PushPullTool.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>

#include "FaceGeometryBuilder.h"
#include "FaceExtruder.h"

PushPullTool::PushPullTool()
    : m_Active(false),
    m_StartMouseY(0.0),
    m_CurrentDistance(0.0f),
    m_Sensitivity(0.01f),
    m_InputBuffer(""),
    m_UsingNumericInput(false),
    m_HasCommittedNumericValue(false)
{
}

bool PushPullTool::start(const FaceSelection& selection, GLFWwindow* window)
{
    if (!selection.isValid())
    {
        std::cout << "[PUSH/PULL] Nenhuma face valida selecionada\n";
        return false;
    }

    m_Selection = selection;

    FaceGeometryBuilder builder;
    m_BaseGeometry = builder.build(m_Selection);

    if (!m_BaseGeometry.isValid())
    {
        std::cout << "[PUSH/PULL] Falha ao construir geometria da face\n";
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

    std::cout << "[PUSH/PULL] Ferramenta iniciada\n";
    return true;
}

void PushPullTool::update(GLFWwindow* window)
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

        std::cout << "[PUSH/PULL INPUT] " << m_InputBuffer << "\n";
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

            std::cout << "[PUSH/PULL INPUT] " << m_InputBuffer << "\n";
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

            std::cout << "[PUSH/PULL INPUT] " << m_InputBuffer << "\n";
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

            std::cout << "[PUSH/PULL INPUT] " << m_InputBuffer << "\n";
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

                std::cout << "[PUSH/PULL INPUT] Distancia definida: "
                    << m_CurrentDistance << "\n";
            }
            catch (...)
            {
                std::cout << "[PUSH/PULL INPUT] Valor invalido\n";
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
        std::cout << "[PUSH/PULL] Nenhuma operacao ativa para confirmar\n";
        return false;
    }

    FaceExtruder extruder;
    bool success = extruder.extrude(m_Selection, m_CurrentDistance);

    if (success)
    {
        std::cout << "[PUSH/PULL] Extrusao confirmada | distancia = "
            << m_CurrentDistance << "\n";
    }
    else
    {
        std::cout << "[PUSH/PULL] Falha ao confirmar extrusao\n";
    }

    m_Active = false;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;

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

    std::cout << "[PUSH/PULL] Cancelado\n";
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