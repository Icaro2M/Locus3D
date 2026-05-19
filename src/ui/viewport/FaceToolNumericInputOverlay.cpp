#include "FaceToolNumericInputOverlay.h"

#include <cstdio>
#include <cstring>
#include <string>

#include <imgui.h>

FaceToolNumericInputOverlay::FaceToolNumericInputOverlay(
    AppEventBus* eventBus,
    UIContext* context
)
    : m_eventBus(eventBus),
    m_context(context),
    m_lastSyncedValue(0.0f),
    m_wasVisible(false),
    m_userEditing(false)
{
    m_buffer[0] = '\0';
}

void FaceToolNumericInputOverlay::draw()
{
    if (m_context == nullptr || !m_context->faceToolNumericInputVisible)
    {
        m_wasVisible = false;
        m_userEditing = false;
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();

    constexpr float windowWidth = 126.0f;
    constexpr float margin = 16.0f;

    const float panelWidth =
        m_context->rightSidePanelWidth > 0.0f
        ? m_context->rightSidePanelWidth
        : 0.0f;

    ImGui::SetNextWindowPos(ImVec2(
        viewport->WorkPos.x + viewport->WorkSize.x - panelWidth - windowWidth - margin,
        viewport->WorkPos.y + viewport->WorkSize.y - 42.0f - margin
    ));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, 42.0f));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);

    if (ImGui::Begin("FaceToolNumericInputOverlay", nullptr, flags))
    {
        bool syncedBufferThisFrame = false;

        if (!m_wasVisible)
        {
            syncBufferFromContext();
            syncedBufferThisFrame = true;
            m_wasVisible = true;
            m_userEditing = false;
        }

        if (!m_userEditing && m_lastSyncedValue != m_context->faceToolNumericInputValue)
        {
            syncBufferFromContext();
            syncedBufferThisFrame = true;
        }

        const bool handledGlobalKeyboardInput = handleGlobalKeyboardInput();

        ImGui::SetNextItemWidth(-1.0f);

        ImGuiInputTextFlags inputFlags =
            ImGuiInputTextFlags_CharsDecimal |
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_AutoSelectAll;

        const bool enterPressed = ImGui::InputText(
            "##face_tool_numeric_value",
            m_buffer,
            sizeof(m_buffer),
            inputFlags
        );

        if (ImGui::IsItemActivated())
        {
            m_userEditing = true;
        }

        if (!handledGlobalKeyboardInput &&
            !ImGui::IsItemActive() &&
            !enterPressed &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_userEditing = false;
        }

        const bool userChangedInput =
            m_userEditing &&
            ImGui::IsItemEdited() &&
            !syncedBufferThisFrame;

        if ((userChangedInput || enterPressed) && m_eventBus != nullptr)
        {
            m_userEditing = true;
            emitBufferChanged();
        }

        if (enterPressed)
        {
            m_userEditing = false;
            emitConfirm();
        }
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
}

void FaceToolNumericInputOverlay::syncBufferFromContext()
{
    std::snprintf(
        m_buffer,
        sizeof(m_buffer),
        "%.3f",
        m_context != nullptr ? m_context->faceToolNumericInputValue : 0.0f
    );

    m_lastSyncedValue =
        m_context != nullptr ? m_context->faceToolNumericInputValue : 0.0f;
}

bool FaceToolNumericInputOverlay::handleGlobalKeyboardInput()
{
    if (ImGui::IsAnyItemActive() || ImGui::GetIO().WantTextInput)
    {
        return false;
    }

    for (int i = 0; i <= 9; ++i)
    {
        const ImGuiKey numberKey = static_cast<ImGuiKey>(ImGuiKey_0 + i);
        const ImGuiKey keypadKey = static_cast<ImGuiKey>(ImGuiKey_Keypad0 + i);

        if (ImGui::IsKeyPressed(numberKey) || ImGui::IsKeyPressed(keypadKey))
        {
            const char digit = static_cast<char>('0' + i);

            if (m_userEditing)
            {
                appendKeyboardChar(digit);
            }
            else
            {
                beginKeyboardEdit(digit);
            }

            return true;
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Period) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadDecimal))
    {
        if (m_userEditing)
        {
            appendKeyboardChar('.');
        }
        else
        {
            beginKeyboardEdit('.');
        }

        return true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Minus) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract))
    {
        if (m_userEditing)
        {
            if (m_buffer[0] == '-')
            {
                std::memmove(m_buffer, m_buffer + 1, std::strlen(m_buffer));
            }
            else
            {
                const size_t length = std::strlen(m_buffer);

                if (length + 1 < sizeof(m_buffer))
                {
                    std::memmove(m_buffer + 1, m_buffer, length + 1);
                    m_buffer[0] = '-';
                }
            }

            emitBufferChanged();
        }
        else
        {
            beginKeyboardEdit('-');
        }

        return true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
    {
        if (m_userEditing)
        {
            removeLastKeyboardChar();
        }

        return true;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_Enter) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter))
    {
        m_userEditing = false;
        emitConfirm();
        return true;
    }

    return false;
}

void FaceToolNumericInputOverlay::beginKeyboardEdit(char value)
{
    m_buffer[0] = '\0';
    m_userEditing = true;

    if (value == '.')
    {
        appendKeyboardChar('0');
        appendKeyboardChar('.');
        return;
    }

    appendKeyboardChar(value);
}

void FaceToolNumericInputOverlay::appendKeyboardChar(char value)
{
    if (value == '.' && hasDecimalPoint())
    {
        return;
    }

    const size_t length = std::strlen(m_buffer);

    if (length + 1 >= sizeof(m_buffer))
    {
        return;
    }

    m_buffer[length] = value;
    m_buffer[length + 1] = '\0';

    emitBufferChanged();
}

void FaceToolNumericInputOverlay::removeLastKeyboardChar()
{
    const size_t length = std::strlen(m_buffer);

    if (length == 0)
    {
        m_userEditing = false;
        return;
    }

    m_buffer[length - 1] = '\0';

    if (m_buffer[0] == '\0')
    {
        m_userEditing = false;
        syncBufferFromContext();
        return;
    }

    emitBufferChanged();
}

void FaceToolNumericInputOverlay::emitBufferChanged()
{
    if (m_eventBus == nullptr)
    {
        return;
    }

    m_eventBus->emit(
        EventType::FaceToolNumericInputChanged,
        0u,
        std::string(m_buffer)
    );
}

void FaceToolNumericInputOverlay::emitConfirm()
{
    if (m_eventBus == nullptr)
    {
        return;
    }

    m_eventBus->emit(EventType::FaceToolNumericInputConfirmed);
}

bool FaceToolNumericInputOverlay::hasDecimalPoint() const
{
    return std::strchr(m_buffer, '.') != nullptr;
}
