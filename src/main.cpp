#include "core/WindowManager.h"
#include "application/EditorApplication.h"
#include "ui/UILayer.h"
// Inclua os headers do ImGui (certifique-se de tê-los configurados no projeto)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main()
{
    // 1. Janela primeiro
    WindowManager window(1600, 1200, "Locus3D");
    glEnable(GL_DEPTH_TEST);

    // 2. Inicialização do ImGui (AQUI É O SEGREDO)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); // Isso cria o contexto global GImGui que o erro reclama
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Conecta ao GLFW e OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window.getWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    // 3. Agora sim, suas camadas podem ser criadas
    EditorApplication app(&window);
    UILayer uiLayer(app.getEventBus(), app.getUIContext());

    while (!window.shouldClose())
    {
        // 4. Início do frame do ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.processInput();
        app.update();
        app.render();

        // 5. Renderização da UI
        uiLayer.draw();

        // 6. Finalização e Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.pollEvents();
        window.swapBuffers();
    }

    // 7. Limpeza (Importante para evitar memory leaks)
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}