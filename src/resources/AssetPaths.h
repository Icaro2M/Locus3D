#pragma once

#include <string>

class AssetPaths
{
public:
    static std::string shader(const std::string& relativePath)
    {
        return "assets/shaders/" + relativePath;
    }

    static std::string icon(const std::string& relativePath)
    {
        return "assets/icons/" + relativePath;
    }

    static std::string toolbarIcon(const std::string& fileName)
    {
        return icon("toolbar/" + fileName);
    }

    static std::string primitiveIcon(const std::string& fileName)
    {
        return icon("primitives/" + fileName);
    }

    static std::string miscIcon(const std::string& fileName)
    {
        return icon("misc/" + fileName);
    }
};