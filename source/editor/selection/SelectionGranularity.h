/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    /**
     * @brief Selection granularity used by editor interaction and mesh editing.
     */
    enum class SelectionGranularity {
        /**
         * @brief Select complete scene objects.
         */
        Object,

        /**
         * @brief Select mesh vertices.
         */
        Vertex,

        /**
         * @brief Select mesh edges.
         */
        Edge,

        /**
         * @brief Select mesh loops.
         */
        Loop,

        /**
         * @brief Select mesh faces.
         */
        Face
    };

    /**
     * @brief Checks whether a granularity targets mesh components.
     *
     * @param granularity Granularity to inspect.
     * @return True when the granularity is vertex, edge, loop, or face.
     */
    [[nodiscard]] constexpr bool is_mesh_granularity(SelectionGranularity granularity)
    {
        return granularity == SelectionGranularity::Vertex ||
            granularity == SelectionGranularity::Edge ||
            granularity == SelectionGranularity::Loop ||
            granularity == SelectionGranularity::Face;
    }

}