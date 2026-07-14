/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/selection/SelectTool.h"

#include "editor/command/selection/ClearObjectSelectionCommand.h"
#include "editor/command/selection/SelectObjectCommand.h"
#include "editor/command/selection/ToggleObjectSelectionCommand.h"
#include "editor/tools/selection/shapes/PointSelectionShape.h"

#include <memory>
#include <string>
#include <utility>

namespace locus::editor {

    SelectTool::SelectTool()
        : SelectTool(
            std::make_unique<PointSelectionShape>()) {
    }

    SelectTool::SelectTool(
        std::unique_ptr<ISelectionShape> shape)
        : ModalTool(make_descriptor()),
        shape_(std::move(shape)) {
    }

    ToolDescriptor SelectTool::make_descriptor() {
        return ToolDescriptor{
            ToolId{ Id },
            "Select",
            "Selects scene objects and editable mesh components.",
            ToolCategory::Selection,
            ToolCapabilities::ObjectMode |
                ToolCapabilities::MeshMode |
                ToolCapabilities::UsesPointer
        };
    }

    const ISelectionShape* SelectTool::shape() const {
        return shape_.get();
    }

    bool SelectTool::can_activate_tool(
        const ToolContext& context) const {

        (void)context;
        return shape_ != nullptr;
    }

    ToolResult SelectTool::on_event(
        ToolContext& context,
        const ToolEvent& event) {

        switch (event.type) {
        case ToolEventType::PointerMove:
            return update_hover(
                context,
                event);

        case ToolEventType::PointerPress:
            if (event.button !=
                ToolPointerButton::Primary) {

                return ToolResult::ignored();
            }

            return apply_point_selection(
                context,
                event);

        default:
            return ToolResult::ignored();
        }
    }

    ToolResult SelectTool::on_confirm(
        ToolContext& context) {

        (void)context;
        return ToolResult::ignored();
    }

    ToolResult SelectTool::on_cancel(
        ToolContext& context,
        ToolCancelReason reason) {

        (void)context;
        (void)reason;

        return ToolResult::ignored();
    }

    ToolResult SelectTool::update_hover(
        ToolContext& context,
        const ToolEvent& event) {

        if (!shape_) {
            return ToolResult::fail(
                "The selection tool has no selection shape.");
        }

        const SelectionShapeResult hit =
            shape_->resolve(
                context,
                event);

        SceneNodeId hovered{};

        if (hit.has_objects()) {
            hovered = hit.objects.front();
        }

        const SceneNodeId previousHovered =
            context.selection().objects().hovered();

        if (previousHovered == hovered) {
            return ToolResult::ignored();
        }

        const bool changed =
            context.selection_controller()
            .set_hovered_object(hovered);

        if (!changed) {
            return ToolResult::ignored();
        }

        return ToolResult::consumed(
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render,
            hovered.is_valid()
            ? "Hovered object updated."
            : "Hovered object cleared.");
    }

    ToolResult SelectTool::apply_point_selection(
        ToolContext& context,
        const ToolEvent& event) {

        if (!shape_) {
            return ToolResult::fail(
                "The selection tool has no selection shape.");
        }

        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Selection command services are not available.");
        }

        const SelectionShapeResult hit =
            shape_->resolve(
                context,
                event);

        const bool toggle =
            event.has_modifier(
                ToolModifiers::Toggle);

        const bool additive =
            event.has_modifier(
                ToolModifiers::Additive);

        if (hit.empty()) {
            /*
             * Modified empty clicks preserve the current selection. This matches
             * the expectation that toggle/additive gestures affect only a target.
             */
            if (toggle || additive) {
                return ToolResult::ignored();
            }

            if (context.selection().objects().empty()) {
                return ToolResult::ignored();
            }

            return from_command_result(
                context.execute_command(
                    std::make_unique<
                    ClearObjectSelectionCommand>()),
                "Object selection cleared.");
        }

        const SceneNodeId object =
            hit.objects.front();

        if (toggle || additive) {
            /*
             * There is currently no dedicated AddObjectSelectionCommand. Toggle is
             * therefore the existing command-backed multi-selection operation.
             * A distinct additive command can be introduced when its semantics are
             * required by the application keymap.
             */
            return from_command_result(
                context.execute_command(
                    std::make_unique<
                    ToggleObjectSelectionCommand>(
                        object)),
                "Object selection toggled.");
        }

        if (context.selection().objects().size() == 1u &&
            context.selection().objects().contains(object) &&
            context.selection().objects().active() == object) {

            return ToolResult::ignored();
        }

        return from_command_result(
            context.execute_command(
                std::make_unique<SelectObjectCommand>(
                    object)),
            "Object selected.");
    }

    ToolResult SelectTool::from_command_result(
        CommandResult result,
        const char* successMessage) {

        if (!result.success) {
            return ToolResult::fail(
                std::move(result.message),
                result.dirtyFlags);
        }

        std::string message =
            std::move(result.message);

        if (message.empty() &&
            successMessage != nullptr) {

            message = successMessage;
        }

        return ToolResult::consumed(
            result.dirtyFlags,
            std::move(message));
    }

} // namespace locus::editor