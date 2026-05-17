#include "core/WindowManager.h"
#include "application/EditorApplication.h"
#include "ui/bridge/UILayer.h"
#include "application/controllers/CameraContext.h" 
#include "ui/ImGuiTheme.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <functional>

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
    UILayer uiLayer(app.getEventBus(), app.getUIContext(), app.getWindowController());

    window.setScrollCallback([&app](double xOffset, double yOffset)
        {
            (void)xOffset;
            app.getCameraContext().getController().processScroll(
                app.getCameraContext().getCamera(),
                static_cast<float>(yOffset)
            );
        });

    std::function<void(bool)> drawFrame = [&](bool processInput)
        {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (processInput)
            {
                app.processInput();
            }

            app.update();
            app.render();

            uiLayer.draw();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            window.swapBuffers();
        };

    window.setRefreshCallback([&drawFrame]()
        {
            drawFrame(false);
        });

    while (!window.shouldClose())
    {
        window.pollEvents();
        drawFrame(true);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return 0;
}
