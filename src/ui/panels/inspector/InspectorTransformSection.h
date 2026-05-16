#pragma once

#include "InspectorModels.h"

class AppEventBus;
struct UIContext;

class InspectorTransformSection
{
public:
    InspectorTransformSection(AppEventBus* eventBus, UIContext* context);

    void draw(const InspectorState& state);

private:
    bool drawVec3Control(const char* label, float* values);

private:
    AppEventBus* m_eventBus = nullptr;
    UIContext* m_context = nullptr;
};