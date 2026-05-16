#pragma once

#include "../../../application/AppEventBus.h"

class FileMenu
{
public:
    explicit FileMenu(AppEventBus* eventBus);

    void draw();

private:
    AppEventBus* m_eventBus;
};