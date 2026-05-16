#pragma once

#include "CustomSolidPanelState.h"
#include "previews/CustomSolidSidePreview.h"
#include "previews/CustomSolidTopPreview.h"

#include "../../bridge/UIContext.h"
#include "../../../application/AppEventBus.h"

class CustomSolidPanel
{
public:
    CustomSolidPanel(AppEventBus* eventBus, UIContext* context);

    void draw();

private:
    void drawHeader();
    void drawPreviews();
    void drawFields();
    void drawActions();

    void clampState();
    void submit();

private:
    AppEventBus* m_eventBus = nullptr;
    UIContext* m_context = nullptr;

    CustomSolidPanelState m_state;

    CustomSolidTopPreview m_topPreview;
    CustomSolidSidePreview m_sidePreview;
};