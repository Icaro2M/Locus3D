/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <cstdint>

namespace locus::kernel::geometry
{
    /**
     * @brief Unsigned storage type used for indexing LEM element arrays.
     */
    using LEMIndex = std::uint32_t;

    /**
     * @brief Unsigned storage type used for LEM element counts.
     */
    using LEMElementCount = std::uint32_t;

    /**
     * @brief Enumerates the element categories stored by LEM.
     */
    enum class LEMElementType
    {
        /**
         * @brief Vertex element.
         */
        Vertex,

        /**
         * @brief Edge element.
         */
        Edge,

        /**
         * @brief Loop element.
         */
        Loop,

        /**
         * @brief Face element.
         */
        Face
    };
}
