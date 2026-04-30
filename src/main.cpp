#include "core/WindowManager.h"
#include "application/EditorApplication.h"
#include "ui/UILayer.h"
#include "application/controllers/CameraContext.h" // Adicionado para manter a câmera funcionando

// Inclua os headers do ImGui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main()
{
    // 1. Janela primeiro
    WindowManager window(1600, 1200, "Locus3D");
    glEnable(GL_DEPTH_TEST);

    // 2. Inicialização do ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); 
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // ==========================================================
    // 3. CARREGAMENTO DA FONTE PERSONALIZADA
    // O caminho deve ser relativo ao local onde o executável roda
    // (Por exemplo: na pasta out/build/x64-debug/)
    // ==========================================================
    io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 16.0f);
    
    // Se quiser uma fonte maior para títulos, você pode carregar duas:
    // ImFont* titleFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Bold.ttf", 20.0f);

    // 4. Conecta ao GLFW e OpenGL
    ImGui_ImplGlfw_InitForOpenGL(window.getWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    // 5. Suas camadas e callbacks
    EditorApplication app(&window);
    UILayer uiLayer(app.getEventBus(), app.getUIContext());

    // Não esqueça de manter os callbacks da câmera aqui na main!
    glfwSetWindowUserPointer(window.getWindow(), &app.getCameraContext());
    glfwSetScrollCallback(window.getWindow(), CameraContext::scrollCallback);

    while (!window.shouldClose())
    {
        // 6. Início do frame do ImGui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.processInput();
        app.update();
        app.render();

        // 7. Renderização da UI
        uiLayer.draw();

        // 8. Finalização e Render
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.pollEvents();
        window.swapBuffers();
    }

    // 9. Limpeza
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}