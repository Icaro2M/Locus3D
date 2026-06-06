/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

namespace locus::graphics
{

    /**
     * @brief GPU draw parameters derived from uploaded mesh data.
     */
    struct MeshDrawData
    {
        /**
         * @brief Primitive topology passed to the draw call.
         */
        PrimitiveTopology topology = PrimitiveTopology::Triangles;

        /**
         * @brief Type of index buffer elements.
         */
        IndexType indexType = IndexType::UInt32;

        /**
         * @brief True when the mesh should be drawn with indexed rendering.
         */
        bool indexed = false;

        /**
         * @brief Number of vertices available for non-indexed drawing.
         */
        u32 vertexCount = 0;

        /**
         * @brief Number of indices available for indexed drawing.
         */
        u32 indexCount = 0;
    };

}
