#pragma once

#include <imgui.h>

#include <string>
#include <unordered_map>

namespace ui
{
    class IconTextureCache
    {
    public:
        static ImTextureID Get(const std::string& path);
        static void Clear();

    private:
        struct TextureData
        {
            unsigned int textureId = 0;
            int width = 0;
            int height = 0;
        };

        static std::unordered_map<std::string, TextureData> s_textures;

        static TextureData LoadTexture(const std::string& path);
    };
}