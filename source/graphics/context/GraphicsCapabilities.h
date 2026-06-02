/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <string>

namespace locus::graphics
{
    /**
     * @brief Describes capabilities reported by the active graphics context.
     */
    struct GraphicsCapabilities
    {
        /**
         * @brief GPU vendor string.
         */
        std::string vendor;

        /**
         * @brief GPU renderer string.
         */
        std::string renderer;

        /**
         * @brief Graphics API version string.
         */
        std::string version;

        /**
         * @brief Shading language version string.
         */
        std::string shadingLanguageVersion;

        /**
         * @brief Graphics API major version.
         */
        i32 majorVersion = 0;

        /**
         * @brief Graphics API minor version.
         */
        i32 minorVersion = 0;

        /**
         * @brief Maximum supported texture dimension.
         */
        i32 maxTextureSize = 0;

        /**
         * @brief Maximum number of vertex attributes.
         */
        i32 maxVertexAttributes = 0;

        /**
         * @brief Maximum number of uniform buffer binding points.
         */
        i32 maxUniformBufferBindings = 0;

        /**
         * @brief True when the backend supports debug output callbacks.
         */
        bool debugOutputSupported = false;
    };

}
