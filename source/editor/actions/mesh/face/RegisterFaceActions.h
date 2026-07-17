/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

namespace locus::editor {

    class ActionRegistry;

    /**
     * @brief Stable identifiers of built-in face actions.
     */
    namespace face_actions {

        /**
         * @brief Reverses the winding of the selected faces.
         */
        inline constexpr std::string_view FlipFaceId =
            "mesh.face.flip";

    } // namespace face_actions

    /**
     * @brief Registers the built-in mesh face actions.
     *
     * Registration fails when one of the action identifiers is already
     * registered or when an action could not be constructed correctly.
     *
     * Actions successfully inserted before a later failure remain registered.
     * Callers should normally register built-in actions only once during editor
     * initialization.
     *
     * @param registry Registry that will own the created actions.
     * @return True when every face action was registered.
     */
    bool register_face_actions(ActionRegistry& registry);

} // namespace locus::editor