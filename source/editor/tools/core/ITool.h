/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/core/ToolContext.h"
#include "editor/tools/core/ToolDescriptor.h"
#include "editor/tools/core/ToolEvent.h"
#include "editor/tools/core/ToolResult.h"
#include "editor/tools/core/ToolState.h"

namespace locus::editor {

    /**
     * @brief Interface implemented by persistent editor interaction tools.
     *
     * Tools represent active interaction modes such as selection, transformation,
     * extrusion, measurement, and primitive creation. Instantaneous editor
     * operations belong to the actions subsystem instead.
     */
    class ITool {
    public:
        /**
         * @brief Destroys the tool.
         */
        virtual ~ITool() = default;

        ITool() = default;
        ITool(const ITool&) = delete;
        ITool& operator=(const ITool&) = delete;
        ITool(ITool&&) = default;
        ITool& operator=(ITool&&) = default;

        /**
         * @brief Returns static metadata describing the tool.
         *
         * @return Tool descriptor.
         */
        [[nodiscard]]
        virtual const ToolDescriptor& descriptor() const = 0;

        /**
         * @brief Returns the current lifecycle state.
         *
         * @return Current tool state.
         */
        [[nodiscard]]
        virtual ToolState state() const = 0;

        /**
         * @brief Checks whether the tool can be activated in the current editor
         * state.
         *
         * @param context Tool context.
         * @return True when activation is currently valid.
         */
        [[nodiscard]]
        virtual bool can_activate(
            const ToolContext& context) const = 0;

        /**
         * @brief Activates the tool.
         *
         * Activation prepares the tool to receive normalized editor events but
         * does not necessarily start a modal interaction.
         *
         * @param context Tool context.
         * @return Activation result.
         */
        virtual ToolResult activate(
            ToolContext& context) = 0;

        /**
         * @brief Deactivates the tool.
         *
         * Implementations must release transient state and cancel or finalize any
         * active interaction according to their explicit policy.
         *
         * @param context Tool context.
         * @return Deactivation result.
         */
        virtual ToolResult deactivate(
            ToolContext& context) = 0;

        /**
         * @brief Handles one normalized editor event.
         *
         * @param context Tool context.
         * @param event Platform-independent event.
         * @return Event handling result.
         */
        virtual ToolResult handle_event(
            ToolContext& context,
            const ToolEvent& event) = 0;

        /**
         * @brief Confirms the current interaction, when one is active.
         *
         * Persistent and undoable changes should be committed through the command
         * and history systems rather than directly by this interface.
         *
         * @param context Tool context.
         * @return Confirmation result.
         */
        virtual ToolResult confirm(
            ToolContext& context) = 0;

        /**
         * @brief Cancels the current interaction, when one is active.
         *
         * Cancellation must restore or discard temporary state without creating
         * an undo history entry.
         *
         * @param context Tool context.
         * @return Cancellation result.
         */
        virtual ToolResult cancel(
            ToolContext& context) = 0;
    };

} // namespace locus::editor