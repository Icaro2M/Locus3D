#pragma once

#include "InspectorModels.h"

#include "InspectorObjectListSection.h"
#include "InspectorTransformSection.h"

struct ImVec2;

class AppEventBus;

struct UIContext;

class InspectorPanel
{
public:
    InspectorPanel(AppEventBus* eventBus, UIContext* context);

    void draw();

private:
    InspectorState buildState() const;

    void drawResizeHandle(const ImVec2& panelPos, float panelHeight);

private:
    AppEventBus* m_eventBus = nullptr;

    UIContext* m_context = nullptr;

    InspectorObjectListSection m_objectListSection;

    InspectorTransformSection m_transformSection;

    float m_panelWidth = 0.0f;
    bool m_hasUserResized = false;
};