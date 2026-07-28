/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/shortcut/Shortcut.h"

#include <vector>

namespace locus::application {

    class InputState;

    /**
     * @brief Resolves normalized keyboard input into semantic actions.
     */
    class ShortcutManager {
    public:
        /**
         * @brief Creates a manager with the default application keymap.
         */
        ShortcutManager();

        /**
         * @brief Replaces current bindings with the default application keymap.
         */
        void reset_to_defaults();

        /**
         * @brief Resolves the first shortcut action triggered this frame.
         *
         * @param input Current normalized input state.
         * @param context Current shortcut context.
         * @return Semantic action, or None when no binding matches.
         */
        [[nodiscard]] ShortcutAction resolve(
            const InputState& input,
            const ShortcutContext& context) const;

        /**
         * @brief Returns configured bindings.
         *
         * @return Read-only binding list.
         */
        [[nodiscard]] const std::vector<ShortcutBinding>&
            bindings() const noexcept;

    private:
        std::vector<ShortcutBinding> bindings_{};
    };

} // namespace locus::application
