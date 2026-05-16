#pragma once

#include "InspectorModels.h"

class AppEventBus;
struct UIContext;

class InspectorObjectListSection
{
public:
    InspectorObjectListSection(AppEventBus* eventBus, UIContext* context);

    void draw(const InspectorState& state);

private:
    void drawObjectRow(const InspectorObjectItem& object);
    void beginRename(const InspectorObjectItem& object);
    void commitRename();

private:
    AppEventBus* m_eventBus = nullptr;
    UIContext* m_context = nullptr;

    int m_renamingObjectId = 0;
    char m_renameBuffer[256];
};