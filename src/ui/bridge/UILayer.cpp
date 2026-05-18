#include "UILayer.h"
#include <imgui.h>

UILayer::UILayer(AppEventBus* eventBus, UIContext* context, WindowController* windowController)
    : m_eventBus(eventBus),
    m_context(context),
    m_windowController(windowController),
    m_topMenuBar(eventBus, context, windowController),
    m_mainToolbar(eventBus, context),
    m_gizmoToolbar(eventBus, context),
    m_inspectorPanel(eventBus, context),
    m_customSolidPanel(eventBus, context),
    m_primitivesMenu(eventBus, context),
    m_viewportOverlay(context)
{
}

void UILayer::draw()
{
    m_topMenuBar.draw();

    m_mainToolbar.draw();
    m_gizmoToolbar.draw();
    m_viewportOverlay.draw();

    if (m_context->showCustomSolidPanel)
    {
        m_customSolidPanel.draw();
    }
    else
    {
        m_inspectorPanel.draw();
    }
}
