#pragma once

#include "UIContext.h"
#include "../../application/AppEventBus.h"
#include "../toolbar/MainToolbar.h"
#include "../toolbar/GizmoToolbar.h"
#include "../panels/InspectorPanel.h"
#include "../panels/TransformPanel.h"
#include "../panels/primitives/CustomSolidPanel.h"
#include "../panels/primitives/PrimitivesMenu.h"
#include "../viewport/ViewportOverlay.h"
#include "../menu/TopMenuBar.h"

class UILayer {
public:
    UILayer(AppEventBus* eventBus, UIContext* context);
    ~UILayer() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;

    TopMenuBar m_topMenuBar;
    MainToolbar m_mainToolbar;
    GizmoToolbar m_gizmoToolbar;

    InspectorPanel m_inspectorPanel;
    TransformPanel m_transformPanel;
    CustomSolidPanel m_customSolidPanel;
    PrimitivesMenu m_primitivesMenu;
    ViewportOverlay m_viewportOverlay;
};