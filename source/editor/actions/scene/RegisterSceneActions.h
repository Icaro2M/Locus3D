/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string_view>

namespace locus::editor {

    class ActionRegistry;

    /**
     * @brief Stable identifiers of built-in scene actions.
     */
    namespace scene_actions {

        /**
         * @brief Deletes selected scene objects.
         */
        inline constexpr std::string_view DeleteObjectsId =
            "scene.object.delete";

    } // namespace scene_actions

    /**
     * @brief Registers built-in scene actions.
     *
     * @param registry Registry that owns created actions.
     * @return True when every scene action was registered.
     */
    bool register_scene_actions(ActionRegistry& registry);

} // namespace locus::editor
