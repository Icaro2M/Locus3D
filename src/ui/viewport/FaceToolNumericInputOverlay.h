#pragma once

#include "../bridge/UIContext.h"
#include "../../application/AppEventBus.h"

class FaceToolNumericInputOverlay
{
public:
    FaceToolNumericInputOverlay(AppEventBus* eventBus, UIContext* context);
    ~FaceToolNumericInputOverlay() = default;

    void draw();

private:
    AppEventBus* m_eventBus;
    UIContext* m_context;

    char m_buffer[64];
    float m_lastSyncedValue;
    bool m_wasVisible;
    bool m_userEditing;

    void syncBufferFromContext();
    bool handleGlobalKeyboardInput();
    void beginKeyboardEdit(char value);
    void appendKeyboardChar(char value);
    void removeLastKeyboardChar();
    void emitBufferChanged();
    void emitConfirm();
    bool hasDecimalPoint() const;
};
