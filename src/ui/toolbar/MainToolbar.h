#pragma once

#include "../UIContext.h"
#include "../../application/AppEventBus.h"

class MainToolbar {
public:
    MainToolbar(AppEventBus* eventBus, UIContext* context);
    ~MainToolbar() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
    uint32_t m_iconCubeTex = 0;
};