/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

namespace locus::editor {

    class ToolRegistry;

    /**
     * @brief Registers the editor tools available in a default document.
     *
     * @param registry Registry that receives built-in tool factories.
     * @return True when every tool was registered.
     */
    bool register_builtin_tools(
        ToolRegistry& registry);

} // namespace locus::editor
