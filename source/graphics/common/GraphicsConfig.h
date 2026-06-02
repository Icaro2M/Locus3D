#pragma once

#include "GraphicsTypes.h"

#include <string>

namespace locus::graphics
{

    struct GraphicsConfig
    {
        GraphicsApi api = GraphicsApi::OpenGL;

        bool enableDebugOutput = true;
        bool enableVSync = true;

        int requestedMajorVersion = 4;
        int requestedMinorVersion = 5;

        bool coreProfile = true;
        bool forwardCompatible = true;

        ColorRGBA defaultClearColor{
            0.08f,
            0.08f,
            0.09f,
            1.0f
        };

        std::string shaderDirectory = "assets/shaders";
    };

}