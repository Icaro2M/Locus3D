#pragma once

#include "InspectorModels.h"
#include "InspectorObjectListSection.h"
#include "InspectorTransformSection.h"

class AppEventBus;
struct UIContext;

class InspectorPanel
{
public:
    InspectorPanel(AppEventBus* eventBus, UIContext* context);

    void draw();

private:
    InspectorState buildState() const;

private:
    AppEventBus* m_eventBus = nullptr;
    UIContext* m_context = nullptr;

    InspectorObjectListSection m_objectListSection;
    InspectorTransformSection m_transformSection;
};