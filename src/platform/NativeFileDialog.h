#pragma once

#include <string>

class NativeFileDialog
{
public:
    static std::string openSaveDialog();
    static std::string openLoadDialog();
};