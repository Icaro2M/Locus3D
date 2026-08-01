/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    class ActionRegistry;

    /**
     * @brief Registers every built-in immediate editor action.
     *
     * Registration is transactional. If one action group fails, all actions
     * inserted by this invocation are removed from the registry.
     *
     * @param registry Registry that will own built-in action instances.
     * @return True when every built-in action was registered.
     */
    bool register_default_actions(ActionRegistry& registry);

} // namespace locus::editor
