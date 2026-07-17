/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

namespace locus::editor {

    class ActionRegistry;

    /**
     * @brief Stable identifiers of built-in mesh topology actions.
     */
    namespace topology_actions {

        /**
         * @brief Subdivides the selected mesh edges.
         */
        inline constexpr std::string_view SubdivideEdgesId =
            "mesh.topology.subdivide_edges";

        /**
         * @brief Subdivides the selected mesh faces.
         */
        inline constexpr std::string_view SubdivideFacesId =
            "mesh.topology.subdivide_faces";

    } // namespace topology_actions

    /**
     * @brief Registers the built-in mesh topology actions.
     *
     * Registration is transactional for this action group. When one action
     * cannot be registered, actions inserted by this invocation are removed.
     *
     * @param registry Registry that will own the created actions.
     * @return True when every topology action was registered.
     */
    bool register_topology_actions(ActionRegistry& registry);

} // namespace locus::editor