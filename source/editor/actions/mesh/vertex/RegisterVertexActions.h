/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

namespace locus::editor {

    class ActionRegistry;

    /**
     * @brief Stable identifiers of built-in mesh vertex actions.
     */
    namespace vertex_actions {

        /**
         * @brief Merges selected vertices at their average position.
         */
        inline constexpr std::string_view MergeAtCenterId =
            "mesh.vertex.merge_at_center";

        /**
         * @brief Merges selected vertices into the first selected vertex.
         */
        inline constexpr std::string_view MergeAtFirstId =
            "mesh.vertex.merge_at_first";

        /**
         * @brief Merges selected vertices into the last selected vertex.
         */
        inline constexpr std::string_view MergeAtLastId =
            "mesh.vertex.merge_at_last";

    } // namespace vertex_actions

    /**
     * @brief Registers all built-in mesh vertex actions.
     *
     * Registration is transactional. If any action cannot be registered,
     * every action inserted by the current invocation is removed.
     *
     * @param registry Registry that will own the created actions.
     * @return True when every vertex action was registered.
     */
    bool register_vertex_actions(ActionRegistry& registry);

} // namespace locus::editor