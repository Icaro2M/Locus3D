/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

namespace locus::editor {

    class ActionRegistry;

    /**
     * @brief Stable identifiers of built-in mesh edge actions.
     */
    namespace edge_actions {

        /**
         * @brief Marks selected edges as fully sharp.
         */
        inline constexpr std::string_view MarkSharpId =
            "mesh.edge.mark_sharp";

        /**
         * @brief Removes the sharp crease from selected edges.
         */
        inline constexpr std::string_view ClearSharpId =
            "mesh.edge.clear_sharp";

        /**
         * @brief Bridges exactly two selected boundary edges.
         */
        inline constexpr std::string_view BridgeId =
            "mesh.edge.bridge";

    } // namespace edge_actions

    /**
     * @brief Registers all built-in mesh edge actions.
     *
     * Registration is transactional. If one action cannot be registered,
     * every action inserted by the current invocation is removed.
     *
     * @param registry Registry that will own the created actions.
     * @return True when every edge action was registered.
     */
    bool register_edge_actions(ActionRegistry& registry);

} // namespace locus::editor
