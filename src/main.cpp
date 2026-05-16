#include "core/WindowManager.h"
#include "application/EditorApplication.h"
#include "ui/bridge/UILayer.h"
#include "application/controllers/CameraContext.h" 
#include "ui/ImGuiTheme.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main()
{
    WindowManager window(1600, 1200, "Locus3D");
    glEnable(GL_DEPTH_TEST);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext(); 
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ApplyModernTheme();

    io.Fonts->AddFontFromFileTTF("assets/fonts/Roboto-Regular.ttf", 16.0f);
    
    ImGui_ImplGlfw_InitForOpenGL(window.getWindow(), true);
    ImGui_ImplOpenGL3_Init("#version 450");

    EditorApplication app(&window);
    UILayer uiLayer(app.getEventBus(), app.getUIContext());

    glfwSetWindowUserPointer(window.getWindow(), &app.getCameraContext());
    glfwSetScrollCallback(window.getWindow(), CameraContext::scrollCallback);

    while (!window.shouldClose())
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        app.processInput();
        app.update();
        app.render();

        uiLayer.draw();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.pollEvents();
        window.swapBuffers();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}