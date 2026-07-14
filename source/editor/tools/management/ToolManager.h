/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/core/ToolId.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/management/ActiveTool.h"
#include "editor/tools/management/ToolRegistry.h"

namespace locus::editor {

    /**
     * @brief Owns and dispatches the currently active editor tool.
     *
     * The manager receives only normalized editor events. Platform input mapping,
     * keymaps, UI capture, viewport focus, and camera routing remain
     * responsibilities of the application layer.
     */
    class ToolManager {
    public:
        /**
         * @brief Creates a tool manager using an external registry.
         *
         * The registry must outlive the manager.
         *
         * @param registry Tool registry used to create active tools.
         */
        explicit ToolManager(const ToolRegistry& registry);

        ToolManager(const ToolManager&) = delete;
        ToolManager& operator=(const ToolManager&) = delete;
        ToolManager(ToolManager&&) = delete;
        ToolManager& operator=(ToolManager&&) = delete;

        /**
         * @brief Activates a registered tool.
         *
         * When another tool is active, it is deactivated before the candidate
         * becomes active. If candidate activation fails, the previous tool is
         * reactivated whenever possible.
         *
         * @param context Tool context.
         * @param id Identifier of the tool to activate.
         * @return Activation result.
         */
        ToolResult activate_tool(
            ToolContext& context,
            const ToolId& id);

        /**
         * @brief Deactivates and removes the current tool.
         *
         * @param context Tool context.
         * @return Deactivation result.
         */
        ToolResult deactivate_tool(ToolContext& context);

        /**
         * @brief Forwards a normalized event to the active tool.
         *
         * Confirm and Cancel events are routed through the corresponding lifecycle
         * methods. Focus loss cancels an active interaction before being forwarded
         * when no interaction was consumed.
         *
         * @param context Tool context.
         * @param event Normalized editor event.
         * @return Event handling result.
         */
        ToolResult handle_event(
            ToolContext& context,
            const ToolEvent& event);

        /**
         * @brief Confirms the current tool interaction.
         *
         * @param context Tool context.
         * @return Confirmation result.
         */
        ToolResult confirm_active(ToolContext& context);

        /**
         * @brief Cancels the current tool interaction.
         *
         * @param context Tool context.
         * @return Cancellation result.
         */
        ToolResult cancel_active(ToolContext& context);

        /**
         * @brief Checks whether a valid active tool exists.
         *
         * @return True when a tool is active.
         */
        [[nodiscard]] bool has_active_tool() const;

        /**
         * @brief Checks whether the given tool is active.
         *
         * @param id Tool identifier to compare.
         * @return True when the identifier matches the active tool.
         */
        [[nodiscard]] bool is_active(const ToolId& id) const;

        /**
         * @brief Returns the active tool identifier.
         *
         * @return Active identifier, or an invalid identifier when empty.
         */
        [[nodiscard]] const ToolId& active_tool_id() const;

        /**
         * @brief Returns the active tool instance.
         *
         * @return Mutable tool pointer, or null when empty.
         */
        [[nodiscard]] ITool* active_tool();

        /**
         * @brief Returns the active tool instance.
         *
         * @return Read-only tool pointer, or null when empty.
         */
        [[nodiscard]] const ITool* active_tool() const;

        /**
         * @brief Returns the registry used by this manager.
         *
         * @return Read-only registry reference.
         */
        [[nodiscard]] const ToolRegistry& registry() const;

    private:
        /**
         * @brief Applies dirty flags carried by a tool result.
         *
         * @param context Tool context.
         * @param result Result to apply.
         * @return Unchanged result.
         */
        ToolResult apply_result(
            ToolContext& context,
            ToolResult result) const;

        const ToolRegistry* registry_ = nullptr;
        ActiveTool active_{};
    };

} // namespace locus::editor