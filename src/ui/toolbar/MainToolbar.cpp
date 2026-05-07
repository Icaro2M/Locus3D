#include "MainToolbar.h"
#include <imgui.h>

MainToolbar::MainToolbar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), m_context(context)
{
}

void MainToolbar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 70.0f)); 

    ImGuiWindowFlags topBarFlags = ImGuiWindowFlags_NoTitleBar | 
                                   ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoScrollbar | 
                                   ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 10.0f));

    if (ImGui::Begin("MainToolbar", nullptr, topBarFlags))
    {
        ImVec2 btnSize(100.0f, 50.0f); 

// --- FERRAMENTAS DE MODO DE FACE ---
        bool isSelecaoActive = (m_context->activeToolId == 0);
        if (isSelecaoActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.30f, 0.45f, 1.0f)); 
        if (ImGui::Button("Selecao\nFace", btnSize)) { 
            m_context->activeToolId = 0; 
            m_context->showCustomSolidPanel = false; 
            // Agora o botão DISPARA A TECLA F (que liga o face mode)
            m_eventBus->emit(EventType::InputKeyF); 
        }
        if (isSelecaoActive) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isExtrusaoActive = (m_context->activeToolId == 1);
        if (isExtrusaoActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.30f, 0.45f, 1.0f));
        if (ImGui::Button("Extrusao\nde Face", btnSize)) { 
            m_context->activeToolId = 1; 
            m_context->showCustomSolidPanel = false; 
            m_eventBus->emit(EventType::InputKeyT);
        }
        if (isExtrusaoActive) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isMoverActive = (m_context->activeToolId == 2);
        if (isMoverActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.30f, 0.45f, 1.0f));
        if (ImGui::Button("Mover\nFace", btnSize)) { 
            m_context->activeToolId = 2; 
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::InputKeyM);
        }
        if (isMoverActive) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isEscalaActive = (m_context->activeToolId == 3);
        if (isEscalaActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.30f, 0.45f, 1.0f));
        if (ImGui::Button("Escala\nde Face", btnSize)) { 
            m_context->activeToolId = 3; 
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::InputKeyS);
        }
        if (isEscalaActive) ImGui::PopStyleColor();

        ImGui::SameLine(0, 20.0f);
        ImGui::TextDisabled("|"); 
        ImGui::SameLine(0, 20.0f);

        ImVec2 primBtnSize(70.0f, 50.0f);
        if (ImGui::Button("+ Cubo", primBtnSize)) m_eventBus->emit(EventType::AddPrimitive, 0);
        ImGui::SameLine();
        if (ImGui::Button("+ Esfera", primBtnSize)) m_eventBus->emit(EventType::AddPrimitive, 1);
        ImGui::SameLine();
        if (ImGui::Button("+ Cone", primBtnSize)) m_eventBus->emit(EventType::AddPrimitive, 2);
        ImGui::SameLine();
        if (ImGui::Button("+ Cilindro", primBtnSize)) m_eventBus->emit(EventType::AddPrimitive, 3);
        
        ImGui::SameLine(0, 20.0f);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0, 20.0f);

        bool isCustomSolidActive = m_context->showCustomSolidPanel;
        if (isCustomSolidActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.30f, 0.45f, 1.0f));
        if (ImGui::Button("Solido\nPersonalizado", ImVec2(110.0f, 50.0f))) { m_context->showCustomSolidPanel = !m_context->showCustomSolidPanel; }
        if (isCustomSolidActive) ImGui::PopStyleColor();
    }
    ImGui::End();
    ImGui::PopStyleVar();
}