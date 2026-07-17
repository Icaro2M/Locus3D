/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

namespace locus::editor {

    class ActionRegistry;

    /**
     * @brief Stable identifiers of built-in mesh face actions.
     */
    namespace face_actions {

        /**
         * @brief Reverses the winding of the selected faces.
         */
        inline constexpr std::string_view FlipFaceId =
            "mesh.face.flip";

        /**
         * @brief Recalculates normals of the selected faces.
         */
        inline constexpr std::string_view RecalculateNormalsId =
            "mesh.face.recalculate_normals";

    } // namespace face_actions

    /**
     * @brief Registers the built-in mesh face actions.
     *
     * Registration is transactional for this action group. When one action
     * cannot be registered, every action inserted by the current invocation
     * is removed.
     *
     * @param registry Registry that will own the created actions.
     * @return True when every face action was registered.
     */
    bool register_face_actions(ActionRegistry& registry);

} // namespace locus::editor