#pragma once

#include "../bridge/UIContext.h"

class ViewportOverlay {
public:
    ViewportOverlay(UIContext* context);
    ~ViewportOverlay() = default;

    void draw();

private:
    UIContext* m_context;
};