#pragma once

#include "../../application/AppEventBus.h"
#include "../bridge/UIContext.h"

class MainToolbar
{
public:
    MainToolbar(AppEventBus* eventBus, UIContext* context);

    void draw();

private:
    void drawSelectionGroup();
    void drawPrimitiveGroup();
    void drawFaceToolGroup();
    void drawCustomSolidGroup();

    void drawSeparator();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
};