/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "GraphicsTypes.h"

#include <string>

namespace locus::graphics
{
    /**
     * @brief Describes the graphics runtime options requested by the application.
     */
    struct GraphicsConfig
    {
        /**
         * @brief Graphics backend selected for the current runtime.
         */
        GraphicsApi api = GraphicsApi::OpenGL;

        /**
         * @brief Enables backend debug diagnostics when supported.
         */
        bool enableDebugOutput = true;

        /**
         * @brief Enables vertical synchronization for presentation.
         */
        bool enableVSync = true;

        /**
         * @brief Requested OpenGL major version.
         */
        int requestedMajorVersion = 4;

        /**
         * @brief Requested OpenGL minor version.
         */
        int requestedMinorVersion = 5;

        /**
         * @brief Requests an OpenGL core profile context.
         */
        bool coreProfile = true;

        /**
         * @brief Requests a forward-compatible OpenGL context.
         */
        bool forwardCompatible = true;

        /**
         * @brief Default clear color used by simple rendering paths.
         */
        ColorRGBA defaultClearColor{
            0.08f,
            0.08f,
            0.09f,
            1.0f
        };

        /**
         * @brief Relative directory used to load shader assets.
         */
        std::string shaderDirectory = "assets/shaders";
    };

}
