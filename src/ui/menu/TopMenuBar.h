#pragma once

#include "../../application/AppEventBus.h"
#include "../bridge/UIContext.h"
#include "menus/FileMenu.h"

class TopMenuBar
{
public:
    TopMenuBar(AppEventBus* eventBus, UIContext* context);

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;

    FileMenu m_fileMenu;
};