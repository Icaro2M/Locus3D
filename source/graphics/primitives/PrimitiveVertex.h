/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <glm/vec3.hpp>

namespace locus::graphics {

    /**
     * @brief CPU-side vertex used to build generic graphical primitives.
     *
     * PrimitiveVertex stores the geometric and visual attributes required to
     * describe points, lines, and triangles before they are converted into a
     * MeshUploadData payload.
     *
     * The type does not represent a GPU resource and carries no editor-specific
     * semantics such as selection, hover, picking, or scene node identity.
     */
    struct PrimitiveVertex {
        /**
         * @brief Vertex position in the coordinate space chosen by the caller.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Vertex normal used by primitives that require surface shading.
         *
         * Points and lines may leave this value at its default when their shaders
         * do not consume normals.
         */
        glm::vec3 normal{ 0.0f, 0.0f, 1.0f };

        /**
         * @brief Linear RGBA vertex color.
         */
        ColorRGBA color{};
    };

} // namespace locus::graphics