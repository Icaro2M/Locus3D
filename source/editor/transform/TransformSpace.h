/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief Coordinate space used by interactive transform sessions.
     */
    enum class TransformSpace {
        /**
         * @brief Applies deltas in each target local coordinate frame.
         */
        Local,

        /**
         * @brief Applies deltas in editor world coordinates.
         */
        World
    };

} // namespace locus::editor