#pragma once

#include "UIContext.h"
#include "../../application/AppEventBus.h"
#include "../../application/controllers/WindowController.h"
#include "../toolbar/MainToolbar.h"
#include "../toolbar/GizmoToolbar.h"
#include "../panels/inspector/InspectorPanel.h"
#include "../panels/primitives/CustomSolidPanel.h"
#include "../panels/primitives/PrimitivesMenu.h"
#include "../viewport/ViewportOverlay.h"
#include "../menu/TopMenuBar.h"

class UILayer {
public:
    UILayer(AppEventBus* eventBus, UIContext* context, WindowController* windowController);
    ~UILayer() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;
    WindowController* m_windowController;

    TopMenuBar m_topMenuBar;
    MainToolbar m_mainToolbar;
    GizmoToolbar m_gizmoToolbar;

    InspectorPanel m_inspectorPanel;
    CustomSolidPanel m_customSolidPanel;
    PrimitivesMenu m_primitivesMenu;
    ViewportOverlay m_viewportOverlay;
};
