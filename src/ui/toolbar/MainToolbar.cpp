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
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 50.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoScrollbar | 
                             ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));

    if (ImGui::Begin("MainToolbar", nullptr, flags))
    {
        // =========================================================
        // --- FERRAMENTAS DE TRANSFORMAÇÃO GERAL (GIZMO) ---
        // =========================================================
        
        // MOVER (W)
        bool isTranslateActive = (m_context->activeTransformMode == TransformMode::Translate);
        if (isTranslateActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f)); // Cor verde para destaque
        if (ImGui::Button("Mover (W)", ImVec2(90, 30))) { 
            m_context->activeTransformMode = TransformMode::Translate; 
            m_eventBus->emit(EventType::InputKeyW, 0); // Avisa o motor que o modo mudou
        }
        if (isTranslateActive) ImGui::PopStyleColor();

        ImGui::SameLine();

        // GIRAR (E)
        bool isRotateActive = (m_context->activeTransformMode == TransformMode::Rotate);
        if (isRotateActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
        if (ImGui::Button("Girar (E)", ImVec2(90, 30))) { 
            m_context->activeTransformMode = TransformMode::Rotate; 
            m_eventBus->emit(EventType::InputKeyE, 0);
        }
        if (isRotateActive) ImGui::PopStyleColor();

        ImGui::SameLine();

        // ESCALA (R)
        bool isScaleActive = (m_context->activeTransformMode == TransformMode::Scale);
        if (isScaleActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.3f, 1.0f));
        if (ImGui::Button("Escala (R)", ImVec2(90, 30))) { 
            m_context->activeTransformMode = TransformMode::Scale; 
            m_eventBus->emit(EventType::InputKeyR, 0);
        }
        if (isScaleActive) ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::Text("  |  "); // Separador visual
        ImGui::SameLine();

        // =========================================================
        // --- FERRAMENTAS DE MODO DE FACE ---
        // =========================================================
        bool isSelecaoActive = (m_context->activeToolId == 0);
        if (isSelecaoActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Selecao Face", ImVec2(100, 30))) { m_context->activeToolId = 0; m_context->showCustomSolidPanel = false; }
        if (isSelecaoActive) ImGui::PopStyleColor();

        ImGui::SameLine();

        bool isExtrusaoActive = (m_context->activeToolId == 1);
        if (isExtrusaoActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 1.0f));
        if (ImGui::Button("Extrusao", ImVec2(80, 30))) { m_context->activeToolId = 1; m_context->showCustomSolidPanel = false; }
        if (isExtrusaoActive) ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::Text("  |  "); // Separador visual
        ImGui::SameLine();

        // =========================================================
        // --- PRIMITIVAS NATIVAS ---
        // =========================================================
        
        // Botões um pouco mais escuros para diferenciar das ferramentas
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
        
        if (ImGui::Button("+ Cubo", ImVec2(70, 30))) m_eventBus->emit(EventType::AddPrimitive, 0);
        ImGui::SameLine();
        if (ImGui::Button("+ Esfera", ImVec2(70, 30))) m_eventBus->emit(EventType::AddPrimitive, 1);
        ImGui::SameLine();
        if (ImGui::Button("+ Cone", ImVec2(70, 30))) m_eventBus->emit(EventType::AddPrimitive, 2);
        ImGui::SameLine();
        if (ImGui::Button("+ Cilindro", ImVec2(80, 30))) m_eventBus->emit(EventType::AddPrimitive, 3);
        
        ImGui::PopStyleColor();

        // =========================================================
        // --- PAINEL LATERAL (SÓLIDO PERSONALIZADO) ---
        // =========================================================
        ImGui::SameLine(ImGui::GetWindowWidth() - 170.0f);

        bool isCustomSolidActive = m_context->showCustomSolidPanel;
        if (isCustomSolidActive) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.4f, 0.1f, 1.0f));
        if (ImGui::Button("Solido Personalizado", ImVec2(160, 30)))
        {
            m_context->showCustomSolidPanel = !m_context->showCustomSolidPanel;
        }
        if (isCustomSolidActive) ImGui::PopStyleColor();
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}