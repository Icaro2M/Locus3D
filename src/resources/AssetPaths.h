#pragma once

#include <string>

class AssetPaths
{
public:
    static std::string shader(const std::string& relativePath)
    {
        return "assets/shaders/" + relativePath;
    }
};