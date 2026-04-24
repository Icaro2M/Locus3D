#pragma once

#include "../../UIContext.h"
#include "../../../application/AppEventBus.h"

class PrimitivesMenu {
public:
    PrimitivesMenu(AppEventBus* eventBus, UIContext* context);
    ~PrimitivesMenu() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
};