#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <string>

namespace locus::graphics
{

    struct GraphicsCapabilities
    {
        std::string vendor;
        std::string renderer;
        std::string version;
        std::string shadingLanguageVersion;

        i32 majorVersion = 0;
        i32 minorVersion = 0;

        i32 maxTextureSize = 0;
        i32 maxVertexAttributes = 0;
        i32 maxUniformBufferBindings = 0;

        bool debugOutputSupported = false;
    };

}