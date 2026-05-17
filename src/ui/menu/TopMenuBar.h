#pragma once

#include "../../application/AppEventBus.h"
#include "../../application/controllers/WindowController.h"
#include "../bridge/UIContext.h"
#include "menus/FileMenu.h"

class TopMenuBar
{
public:
    TopMenuBar(AppEventBus* eventBus, UIContext* context, WindowController* windowController);

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
    WindowController* m_windowController;

    FileMenu m_fileMenu;
};
