#pragma once

#include "../../application/AppEventBus.h"
#include "../bridge/UIContext.h"
#include "MainToolbarItems.h"

class MainToolbar
{
public:
    MainToolbar(AppEventBus* eventBus, UIContext* context);

    void draw();

private:
    void drawToolbarItem(const ui::toolbar::MainToolbarItem& item);
    void drawPrimitiveDropdown(const ui::toolbar::MainToolbarItem& item);
    void drawToolbarBottomBorder();
    void drawSeparator();

    bool isActionActive(ui::toolbar::MainToolbarAction action) const;
    void handleAction(ui::toolbar::MainToolbarAction action);

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
};