/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/management/ToolManager.h"

#include <utility>

namespace locus::editor {

    ToolManager::ToolManager(const ToolRegistry& registry)
        : registry_(&registry) {
    }

    ToolResult ToolManager::activate_tool(
        ToolContext& context,
        const ToolId& id) {

        if (id.is_invalid()) {
            return ToolResult::fail(
                "Cannot activate a tool with an invalid identifier.");
        }

        if (is_active(id)) {
            return ToolResult::consumed(
                EditorDirtyFlags::None,
                "The requested tool is already active.");
        }

        std::unique_ptr<ITool> candidate = registry_->create(id);
        if (!candidate) {
            return ToolResult::fail(
                "The requested tool is not registered or could not be created.");
        }

        if (!candidate->can_activate(context)) {
            return ToolResult::fail(
                "The requested tool cannot be activated in the current editor state.");
        }

        ActiveTool previous = std::move(active_);

        if (previous.is_valid()) {
            ToolResult deactivation =
                previous.instance->deactivate(context);

            apply_result(context, deactivation);

            if (deactivation.failed()) {
                active_ = std::move(previous);

                return ToolResult::fail(
                    "The active tool could not be deactivated.");
            }
        }

        ToolResult activation = candidate->activate(context);
        apply_result(context, activation);

        if (activation.failed()) {
            if (previous.is_valid() &&
                previous.instance->can_activate(context)) {

                ToolResult restoration =
                    previous.instance->activate(context);

                apply_result(context, restoration);

                if (!restoration.failed()) {
                    active_ = std::move(previous);
                }
            }

            return activation;
        }

        active_ = ActiveTool{
            id,
            std::move(candidate)
        };

        return activation;
    }

    ToolResult ToolManager::deactivate_tool(
        ToolContext& context) {

        if (!active_.is_valid()) {
            return ToolResult::ignored();
        }

        ToolResult result =
            active_.instance->deactivate(context);

        result = apply_result(context, std::move(result));

        if (!result.failed()) {
            active_.clear();
        }

        return result;
    }

    ToolResult ToolManager::handle_event(
        ToolContext& context,
        const ToolEvent& event) {

        if (!active_.is_valid()) {
            return ToolResult::ignored();
        }

        switch (event.type) {
        case ToolEventType::Confirm:
            return confirm_active(context);

        case ToolEventType::Cancel:
            return cancel_active(context);

        case ToolEventType::FocusLost:
            if (active_.instance->state() ==
                ToolState::Interacting) {

                return cancel_active(context);
            }

            break;

        default:
            break;
        }

        return apply_result(
            context,
            active_.instance->handle_event(
                context,
                event));
    }

    ToolResult ToolManager::confirm_active(
        ToolContext& context) {

        if (!active_.is_valid()) {
            return ToolResult::ignored();
        }

        return apply_result(
            context,
            active_.instance->confirm(context));
    }

    ToolResult ToolManager::cancel_active(
        ToolContext& context) {

        if (!active_.is_valid()) {
            return ToolResult::ignored();
        }

        return apply_result(
            context,
            active_.instance->cancel(context));
    }

    bool ToolManager::has_active_tool() const {
        return active_.is_valid();
    }

    bool ToolManager::is_active(const ToolId& id) const {
        return active_.is_valid() && active_.id == id;
    }

    const ToolId& ToolManager::active_tool_id() const {
        return active_.id;
    }

    ITool* ToolManager::active_tool() {
        return active_.instance.get();
    }

    const ITool* ToolManager::active_tool() const {
        return active_.instance.get();
    }

    const ToolRegistry& ToolManager::registry() const {
        return *registry_;
    }

    ToolResult ToolManager::apply_result(
        ToolContext& context,
        ToolResult result) const {

        if (result.dirtyFlags != EditorDirtyFlags::None) {
            context.mark_dirty(result.dirtyFlags);
        }

        return result;
    }

} // namespace locus::editor