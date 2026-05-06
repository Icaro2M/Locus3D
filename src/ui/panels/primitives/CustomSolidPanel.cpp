#include "CustomSolidPanel.h"
#include <imgui.h>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>

CustomSolidPanel::CustomSolidPanel(AppEventBus* eventBus, UIContext* context)
    : m_eventBus(eventBus), 
      m_context(context),
      m_sides(5),
      m_bottomRadius(2.5f),
      m_topRadius(2.0f),
      m_height(3.0f)
{
    std::strncpy(m_nameBuffer, "solido", sizeof(m_nameBuffer) - 1);
}

void CustomSolidPanel::draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + viewport->Size.x - 300.0f, viewport->Pos.y + 50.0f));
    ImGui::SetNextWindowSize(ImVec2(300.0f, viewport->Size.y - 50.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoTitleBar;

    if (ImGui::Begin("SolidoPersonalizadoContainer", nullptr, flags))
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Text("Configurar Solido");
        ImGui::SameLine(ImGui::GetWindowWidth() - 30.0f);
        if (ImGui::Button("X"))
        {
            m_context->showCustomSolidPanel = false;
        }

        ImGui::Spacing();
        
        // Aumentei o espaço do Preview para 160 pixels de altura para ficar mais bonito
        if (ImGui::BeginChild("PreviewArea", ImVec2(0, 160), true))
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Preview Visual");
            
            // ==============================================================
            // LÓGICA DE DESENHO DO PREVIEW VISUAL (Wireframe Isométrico)
            // ==============================================================
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 p_min = ImGui::GetCursorScreenPos(); 
            ImVec2 p_max = ImVec2(p_min.x + ImGui::GetContentRegionAvail().x, p_min.y + ImGui::GetContentRegionAvail().y);
            ImVec2 center = ImVec2((p_min.x + p_max.x) * 0.5f, (p_min.y + p_max.y) * 0.5f);

            // Descobre o limite de tamanho para a figura não vazar da caixa
            float max_r = std::max(m_bottomRadius, m_topRadius);
            float max_w = max_r * 2.0f;
            float max_h = m_height;
            if (max_w < 0.1f) max_w = 0.1f; // Evita divisão por zero
            if (max_h < 0.1f) max_h = 0.1f;

            // Calcula a escala adaptativa baseada nos inputs
            float avail_w = p_max.x - p_min.x - 20.0f;
            float avail_h = p_max.y - p_min.y - 20.0f;
            float scale_x = avail_w / max_w;
            float scale_y = avail_h / max_h;
            float scale = std::min(scale_x, scale_y) * 0.75f; // 0.75 deixa uma margem de segurança

            float scaled_h = m_height * scale;
            float scaled_r_bot = m_bottomRadius * scale;
            float scaled_r_top = m_topRadius * scale;

            float y_top = center.y - scaled_h * 0.5f;
            float y_bot = center.y + scaled_h * 0.5f;

            int num_segments = m_sides;
            float angle_step = (2.0f * 3.14159265f) / num_segments;
            
            ImU32 col_lines = IM_COL32(50, 150, 255, 255); // Cor azulada do wireframe

            std::vector<ImVec2> top_pts(num_segments);
            std::vector<ImVec2> bot_pts(num_segments);
            
            // Fator de inclinação da câmera (achata o círculo em elipse para dar ilusão 3D)
            float tilt = 0.35f; 

            for (int i = 0; i < num_segments; ++i) {
                float angle = i * angle_step;
                float cx = std::cos(angle);
                float cz = std::sin(angle); 
                
                top_pts[i] = ImVec2(center.x + cx * scaled_r_top, y_top + cz * scaled_r_top * tilt);
                bot_pts[i] = ImVec2(center.x + cx * scaled_r_bot, y_bot + cz * scaled_r_bot * tilt);
            }

            // Une os pontos desenhando linhas na tela
            for (int i = 0; i < num_segments; ++i) {
                int next_i = (i + 1) % num_segments;
                draw_list->AddLine(top_pts[i], top_pts[next_i], col_lines, 1.5f); // Base superior
                draw_list->AddLine(bot_pts[i], bot_pts[next_i], col_lines, 1.5f); // Base inferior
                draw_list->AddLine(top_pts[i], bot_pts[i], col_lines, 1.5f);      // Arestas laterais
            }
            // ==============================================================
        }
        ImGui::EndChild();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::InputText("Nome", m_nameBuffer, sizeof(m_nameBuffer));
        
        if (ImGui::InputInt("Lados", &m_sides))
        {
            if (m_sides < 3) m_sides = 3; // Impede menos que 3 lados (triângulo)
            if (m_sides > 64) m_sides = 64; // Impede lados demais pra não quebrar a performance
        }
        
        ImGui::DragFloat("Raio Inf.", &m_bottomRadius, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Raio Sup.", &m_topRadius, 0.1f, 0.0f, 100.0f);
        ImGui::DragFloat("Altura", &m_height, 0.1f, 0.1f, 100.0f);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Adicionar", ImVec2(-1, 40)))
        {
            // Salva as configurações para o SceneContext buscar depois
            std::strncpy(m_context->customSolidName, m_nameBuffer, sizeof(m_context->customSolidName) - 1);
            m_context->customSolidSides = m_sides;
            m_context->customSolidBottomRadius = m_bottomRadius;
            m_context->customSolidTopRadius = m_topRadius;
            m_context->customSolidHeight = m_height;

            m_eventBus->emit(EventType::AddCustomSolid, m_sides);
            m_context->showCustomSolidPanel = false;
        }
    }
    ImGui::End();
}