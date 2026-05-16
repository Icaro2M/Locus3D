#include "IconTextureCache.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace ui
{
    std::unordered_map<std::string, IconTextureCache::TextureData> IconTextureCache::s_textures;

    ImTextureID IconTextureCache::Get(const std::string& path)
    {
        auto it = s_textures.find(path);

        if (it != s_textures.end())
        {
            return static_cast<ImTextureID>(it->second.textureId);
        }

        TextureData texture = LoadTexture(path);

        if (texture.textureId == 0)
        {
            return 0;
        }

        unsigned int textureId = texture.textureId;
        s_textures[path] = texture;

        return static_cast<ImTextureID>(textureId);
    }

    void IconTextureCache::Clear()
    {
        for (auto& pair : s_textures)
        {
            if (pair.second.textureId != 0)
            {
                glDeleteTextures(1, &pair.second.textureId);
            }
        }

        s_textures.clear();
    }

    IconTextureCache::TextureData IconTextureCache::LoadTexture(const std::string& path)
    {
        TextureData result;

        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_set_flip_vertically_on_load(false);

        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

        if (data == nullptr)
        {
            return result;
        }

        GLuint textureId = 0;
        glGenTextures(1, &textureId);
        glBindTexture(GL_TEXTURE_2D, textureId);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA,
            width,
            height,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            data
        );

        glBindTexture(GL_TEXTURE_2D, 0);

        stbi_image_free(data);

        result.textureId = textureId;
        result.width = width;
        result.height = height;

        return result;
    }
}