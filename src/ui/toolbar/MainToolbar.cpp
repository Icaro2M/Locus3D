#include "MainToolbar.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <cmath>

MainToolbar::MainToolbar(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), m_context(context)
{
}

void MainToolbar::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(viewport->Pos);
    // Altura reduzida para 75px, pois botões sem texto ocupam menos espaço vertical
    ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, 75.0f)); 

    ImGuiWindowFlags topBarFlags = ImGuiWindowFlags_NoTitleBar | 
                                   ImGuiWindowFlags_NoResize | 
                                   ImGuiWindowFlags_NoMove | 
                                   ImGuiWindowFlags_NoScrollbar | 
                                   ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.13f, 0.15f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15.0f, 10.0f));

    if (ImGui::Begin("MainToolbar", nullptr, topBarFlags))
    {
        ImVec2 btnSize(55.0f, 55.0f); // Botões perfeitamente quadrados!

        // --- FERRAMENTAS DE MODO DE FACE ---
        bool isSelecaoActive = (m_context->activeToolId == 0);
        if (iconButton("##btn_selecao", "Selecao (F)", ToolbarIcon::SelectFace, btnSize, isSelecaoActive)) {
            m_context->activeToolId = 0; 
            m_context->showCustomSolidPanel = false; 
            m_eventBus->emit(EventType::InputKeyF);
        }

        ImGui::SameLine();

        bool isExtrusaoActive = (m_context->activeToolId == 1);
        if (iconButton("##btn_extrusao", "Extrusao de Face (T)", ToolbarIcon::ExtrudeFace, btnSize, isExtrusaoActive)) {
            m_context->activeToolId = 1; 
            m_context->showCustomSolidPanel = false; 
            m_eventBus->emit(EventType::InputKeyT);
        }

        ImGui::SameLine();

        bool isMoverActive = (m_context->activeToolId == 2);
        if (iconButton("##btn_mover", "Mover Face (M)", ToolbarIcon::MoveFace, btnSize, isMoverActive)) {
            m_context->activeToolId = 2; 
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::InputKeyM);
        }

        ImGui::SameLine();

        bool isEscalaActive = (m_context->activeToolId == 3);
        if (iconButton("##btn_escala", "Escala de Face (S)", ToolbarIcon::ScaleFace, btnSize, isEscalaActive)) {
            m_context->activeToolId = 3; 
            m_context->showCustomSolidPanel = false;
            m_eventBus->emit(EventType::InputKeyS);
        }

        ImGui::SameLine(0, 20.0f);
        ImGui::TextDisabled("|"); 
        ImGui::SameLine(0, 20.0f);

        // --- DROPDOWN DE PRIMITIVAS ---
        bool isPopupOpen = ImGui::IsPopupOpen("PopupPrimitivas");
        
        // Botão que abre o menu (usa o ícone de Cubo como representação)
        if (iconButton("##btn_add_prim", "Adicionar Primitiva", ToolbarIcon::Cube, btnSize, isPopupOpen)) {
            ImGui::OpenPopup("PopupPrimitivas");
        }

        // Estilo do Dropdown flutuante
        ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
        ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.15f, 0.16f, 0.18f, 1.0f)); 
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.3f, 0.35f, 1.0f));

        if (ImGui::BeginPopup("PopupPrimitivas"))
        {
            ImVec2 popBtnSize(50.0f, 50.0f); // Botões do grid um pouquinho menores

            // Linha 1: Cubo e Esfera
            if (iconButton("##pop_cubo", "Cubo", ToolbarIcon::Cube, popBtnSize, false)) { 
                m_eventBus->emit(EventType::AddPrimitive, 0); 
                ImGui::CloseCurrentPopup(); 
            }
            ImGui::SameLine();
            if (iconButton("##pop_esfera", "Esfera", ToolbarIcon::Sphere, popBtnSize, false)) { 
                m_eventBus->emit(EventType::AddPrimitive, 1); 
                ImGui::CloseCurrentPopup(); 
            }
            
            // Linha 2: Cone e Cilindro
            if (iconButton("##pop_cone", "Cone", ToolbarIcon::Cone, popBtnSize, false)) { 
                m_eventBus->emit(EventType::AddPrimitive, 2); 
                ImGui::CloseCurrentPopup(); 
            }
            ImGui::SameLine();
            if (iconButton("##pop_cilindro", "Cilindro", ToolbarIcon::Cylinder, popBtnSize, false)) { 
                m_eventBus->emit(EventType::AddPrimitive, 3); 
                ImGui::CloseCurrentPopup(); 
            }

            ImGui::EndPopup();
        }
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
        
        ImGui::SameLine(0, 20.0f);
        ImGui::TextDisabled("|");
        ImGui::SameLine(0, 20.0f);

        // --- SÓLIDO PERSONALIZADO ---
        bool isCustomSolidActive = m_context->showCustomSolidPanel;
        if (iconButton("##btn_solido_custom", "Solido Personalizado", ToolbarIcon::CustomSolid, btnSize, isCustomSolidActive)) {
            m_context->showCustomSolidPanel = !m_context->showCustomSolidPanel;
        }
    }
    ImGui::End();
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}

// ========================================================================
// HELPER: DESENHA O BOTÃO APENAS COM ÍCONE E TOOLTIP
// ========================================================================
bool MainToolbar::iconButton(const char* id, const char* tooltip, ToolbarIcon icon, ImVec2 size, bool active)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImVec2 pos = window->DC.CursorPos;
    
    ImGui::InvisibleButton(id, size);
    bool clicked = ImGui::IsItemClicked();
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();

    // TOOLTIP GARANTIDA AQUI!
    if (hovered && tooltip && tooltip[0] != '\0') {
        ImGui::SetTooltip("%s", tooltip);
    }

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    ImU32 bgColor = IM_COL32(30, 32, 36, 255);       
    ImU32 borderColor = IM_COL32(50, 52, 58, 255);   
    ImU32 iconColor = IM_COL32(200, 200, 205, 255);  

    if (active) {
        bgColor = IM_COL32(42, 65, 110, 255);        
        borderColor = IM_COL32(80, 110, 170, 255);
        iconColor = IM_COL32(255, 255, 255, 255);    
    } else if (held) {
        bgColor = IM_COL32(20, 22, 25, 255);
    } else if (hovered) {
        bgColor = IM_COL32(45, 48, 55, 255);
        iconColor = IM_COL32(255, 255, 255, 255);
    }

    // Fundo
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), bgColor, 6.0f);
    drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), borderColor, 6.0f, 0, 1.0f);

    // Ícone 100% centralizado
    ImVec2 iconCenter(pos.x + size.x * 0.5f, pos.y + size.y * 0.5f);
    float iconScale = size.y * 0.35f; 
    
    drawToolbarIcon(drawList, icon, iconCenter, iconScale, iconColor);

    return clicked;
}

// ========================================================================
// DESENHO VETORIAL DOS ÍCONES
// ========================================================================
void MainToolbar::drawToolbarIcon(ImDrawList* drawList, ToolbarIcon icon, ImVec2 center, float s, ImU32 color)
{
    float cx = center.x;
    float cy = center.y;
    float lineW = 1.8f; 

    switch (icon)
    {
        case ToolbarIcon::SelectFace: {
            ImVec2 points[7] = {
                ImVec2(cx - s*0.4f, cy - s*0.8f), ImVec2(cx - s*0.4f, cy + s*0.8f),
                ImVec2(cx - s*0.1f, cy + s*0.5f), ImVec2(cx + s*0.3f, cy + s*0.9f),
                ImVec2(cx + s*0.5f, cy + s*0.7f), ImVec2(cx + s*0.1f, cy + s*0.3f),
                ImVec2(cx + s*0.6f, cy + s*0.1f)
            };
            drawList->AddPolyline(points, 7, color, ImDrawFlags_Closed, lineW);
            break;
        }
        case ToolbarIcon::ExtrudeFace: {
            drawList->AddRect(ImVec2(cx - s*0.7f, cy), ImVec2(cx + s*0.7f, cy + s*0.8f), color, 0.0f, 0, lineW);
            drawList->AddRectFilled(ImVec2(cx - s*0.5f, cy - s*0.8f), ImVec2(cx + s*0.5f, cy - s*0.2f), IM_COL32(100, 150, 255, 100));
            drawList->AddRect(ImVec2(cx - s*0.5f, cy - s*0.8f), ImVec2(cx + s*0.5f, cy - s*0.2f), color, 0.0f, 0, lineW);
            drawList->AddLine(ImVec2(cx, cy - s*0.2f), ImVec2(cx, cy + s*0.2f), color, lineW); 
            break;
        }
        case ToolbarIcon::MoveFace: {
            drawList->AddLine(ImVec2(cx, cy - s), ImVec2(cx, cy + s), color, lineW);
            drawList->AddLine(ImVec2(cx - s, cy), ImVec2(cx + s, cy), color, lineW);
            drawList->AddTriangleFilled(ImVec2(cx, cy - s*1.2f), ImVec2(cx - s*0.3f, cy - s*0.8f), ImVec2(cx + s*0.3f, cy - s*0.8f), color);
            drawList->AddTriangleFilled(ImVec2(cx, cy + s*1.2f), ImVec2(cx - s*0.3f, cy + s*0.8f), ImVec2(cx + s*0.3f, cy + s*0.8f), color);
            drawList->AddTriangleFilled(ImVec2(cx - s*1.2f, cy), ImVec2(cx - s*0.8f, cy - s*0.3f), ImVec2(cx - s*0.8f, cy + s*0.3f), color);
            drawList->AddTriangleFilled(ImVec2(cx + s*1.2f, cy), ImVec2(cx + s*0.8f, cy - s*0.3f), ImVec2(cx + s*0.8f, cy + s*0.3f), color);
            break;
        }
        case ToolbarIcon::ScaleFace: {
            drawList->AddRect(ImVec2(cx - s*0.6f, cy - s*0.6f), ImVec2(cx + s*0.6f, cy + s*0.6f), color, 0.0f, 0, lineW);
            drawList->AddRectFilled(ImVec2(cx - s*0.8f, cy - s*0.8f), ImVec2(cx - s*0.4f, cy - s*0.4f), color);
            drawList->AddRectFilled(ImVec2(cx + s*0.4f, cy + s*0.4f), ImVec2(cx + s*0.8f, cy + s*0.8f), color);
            drawList->AddLine(ImVec2(cx - s*0.5f, cy - s*0.5f), ImVec2(cx + s*0.5f, cy + s*0.5f), color, lineW);
            break;
        }
        case ToolbarIcon::Cube: {
            float off = s * 0.4f;
            drawList->AddRect(ImVec2(cx - s + off, cy - s + off), ImVec2(cx + s*0.6f + off, cy + s*0.6f + off), color, 0.0f, 0, lineW);
            drawList->AddRect(ImVec2(cx - s - off, cy - s - off), ImVec2(cx + s*0.6f - off, cy + s*0.6f - off), color, 0.0f, 0, lineW);
            drawList->AddLine(ImVec2(cx - s + off, cy - s + off), ImVec2(cx - s - off, cy - s - off), color, lineW);
            drawList->AddLine(ImVec2(cx + s*0.6f + off, cy - s + off), ImVec2(cx + s*0.6f - off, cy - s - off), color, lineW);
            drawList->AddLine(ImVec2(cx - s + off, cy + s*0.6f + off), ImVec2(cx - s - off, cy + s*0.6f - off), color, lineW);
            drawList->AddLine(ImVec2(cx + s*0.6f + off, cy + s*0.6f + off), ImVec2(cx + s*0.6f - off, cy + s*0.6f - off), color, lineW);
            break;
        }
        case ToolbarIcon::Sphere: {
            drawList->AddCircle(center, s, color, 0, lineW);
            drawList->AddLine(ImVec2(cx - s, cy), ImVec2(cx + s, cy), color, lineW * 0.5f); 
            break;
        }
        case ToolbarIcon::Cone: {
            drawList->AddTriangle(ImVec2(cx, cy - s), ImVec2(cx - s*0.8f, cy + s*0.6f), ImVec2(cx + s*0.8f, cy + s*0.6f), color, lineW);
            drawList->AddLine(ImVec2(cx - s*0.8f, cy + s*0.6f), ImVec2(cx + s*0.8f, cy + s*0.6f), color, lineW); 
            drawList->AddLine(ImVec2(cx, cy - s), ImVec2(cx, cy + s*0.6f), color, lineW * 0.5f); 
            break;
        }
        case ToolbarIcon::Cylinder: {
            drawList->AddRect(ImVec2(cx - s*0.6f, cy - s), ImVec2(cx + s*0.6f, cy + s), color, 0.0f, 0, lineW);
            drawList->AddLine(ImVec2(cx - s*0.6f, cy - s*0.6f), ImVec2(cx + s*0.6f, cy - s*0.6f), color, lineW); 
            drawList->AddLine(ImVec2(cx - s*0.6f, cy + s*0.6f), ImVec2(cx + s*0.6f, cy + s*0.6f), color, lineW); 
            break;
        }
        case ToolbarIcon::CustomSolid: {
            ImVec2 points[5] = {
                ImVec2(cx, cy - s), ImVec2(cx + s, cy - s*0.2f),
                ImVec2(cx + s*0.6f, cy + s), ImVec2(cx - s*0.6f, cy + s),
                ImVec2(cx - s, cy - s*0.2f)
            };
            drawList->AddPolyline(points, 5, color, ImDrawFlags_Closed, lineW);
            break;
        }
    }
}