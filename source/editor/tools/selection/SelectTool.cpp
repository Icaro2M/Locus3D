/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/tools/selection/SelectTool.h"

#include "editor/command/mesh/EditMeshSelectionCommand.h"
#include "editor/command/selection/ClearObjectSelectionCommand.h"
#include "editor/command/selection/ClearMeshSelectionCommand.h"
#include "editor/command/selection/SelectObjectCommand.h"
#include "editor/command/selection/SelectMeshComponentCommand.h"
#include "editor/command/selection/SetObjectSelectionCommand.h"
#include "editor/command/selection/ToggleObjectSelectionCommand.h"
#include "editor/command/selection/ToggleMeshComponentSelectionCommand.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "editor/tools/selection/shapes/PointSelectionShape.h"
#include "kernel/geometry/queries/SelectionHit.h"

#include <memory>
#include <string>
#include <utility>
#include <glm/geometric.hpp>

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

    bool SelectTool::is_box_selecting() const noexcept {
        return boxSelecting_;
    }

    ScreenSelectionRect SelectTool::selection_rect() const noexcept {
        return ScreenSelectionRect::from_points(
            capture_.startPosition,
            capture_.currentPosition);
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
            if (capture_.has_pointer()) {
                return update_pointer_selection(
                    context,
                    event);
            }

            return update_hover(
                context,
                event);

        case ToolEventType::PointerPress:
            if (event.button !=
                ToolPointerButton::Primary) {

                return ToolResult::ignored();
            }

            return begin_pointer_selection(
                context,
                event);

        case ToolEventType::PointerRelease:
            if (event.button !=
                ToolPointerButton::Primary) {

                return ToolResult::ignored();
            }

            return finish_pointer_selection(
                context,
                event);

        case ToolEventType::Cancel:
        case ToolEventType::FocusLost:
            return on_cancel(
                context,
                ToolCancelReason::UserRequest);

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

        (void)reason;

        if (boxSelecting_) {
            context.selection_controller().clear_hovered_mesh_component();
            context.selection_controller().set_hovered_object(SceneNodeId{});
        }

        clear_interaction_state();

        return ToolResult::consumed(
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render,
            "Selection interaction cancelled.");
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

        if (is_mesh_granularity(context.selection().granularity())) {
            const bool changed =
                context.selection_controller()
                .set_hovered_mesh_component(hit.component);

            (void)context.selection_controller()
                .set_hovered_object(SceneNodeId{});

            if (!changed) {
                return ToolResult::ignored();
            }

            return ToolResult::consumed(
                EditorDirtyFlags::Selection |
                EditorDirtyFlags::Render,
                hit.component.hit
                ? "Hovered mesh component updated."
                : "Hovered mesh component cleared.");
        }

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

        if (is_mesh_granularity(context.selection().granularity())) {
            if (hit.component.hit) {
                if (hit.componentNode.is_valid() &&
                    hit.componentNode !=
                    context.selection().mesh().active_mesh()) {
                    const bool activated =
                        context.selection_controller()
                        .enter_mesh_context(
                            hit.componentNode,
                            context.selection().granularity());

                    if (!activated) {
                        return ToolResult::fail(
                            "Could not activate the picked mesh context.");
                    }
                }

                switch (hit.component.type) {
                case kernel::geometry::LEMElementType::Vertex:
                    if (toggle || additive) {
                        return from_command_result(
                            context.execute_command(
                                std::make_unique<ToggleMeshComponentSelectionCommand>(
                                    hit.component.vertex)),
                            "Vertex selection toggled.");
                    }

                    return from_command_result(
                        context.execute_command(
                            std::make_unique<SelectMeshComponentCommand>(
                                hit.component.vertex)),
                        "Vertex selected.");

                case kernel::geometry::LEMElementType::Edge:
                    if (toggle || additive) {
                        return from_command_result(
                            context.execute_command(
                                std::make_unique<ToggleMeshComponentSelectionCommand>(
                                    hit.component.edge)),
                            "Edge selection toggled.");
                    }

                    return from_command_result(
                        context.execute_command(
                            std::make_unique<SelectMeshComponentCommand>(
                                hit.component.edge)),
                        "Edge selected.");

                case kernel::geometry::LEMElementType::Loop:
                    if (toggle || additive) {
                        return from_command_result(
                            context.execute_command(
                                std::make_unique<ToggleMeshComponentSelectionCommand>(
                                    hit.component.loop)),
                            "Loop selection toggled.");
                    }

                    return from_command_result(
                        context.execute_command(
                            std::make_unique<SelectMeshComponentCommand>(
                                hit.component.loop)),
                        "Loop selected.");

                case kernel::geometry::LEMElementType::Face:
                    if (toggle || additive) {
                        return from_command_result(
                            context.execute_command(
                                std::make_unique<ToggleMeshComponentSelectionCommand>(
                                    hit.component.face)),
                            "Face selection toggled.");
                    }

                    return from_command_result(
                        context.execute_command(
                            std::make_unique<SelectMeshComponentCommand>(
                                hit.component.face)),
                        "Face selected.");
                }
            }

            if (toggle || additive) {
                return ToolResult::ignored();
            }

            if (context.selection().mesh().empty()) {
                return ToolResult::ignored();
            }

            return from_command_result(
                context.execute_command(
                    std::make_unique<ClearMeshSelectionCommand>()),
                "Mesh component selection cleared.");
        }

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

    ToolResult SelectTool::begin_pointer_selection(
        ToolContext& context,
        const ToolEvent& event)
    {
        (void)context;
        capture_.begin_pointer(
            ToolPointerButton::Primary,
            event.pointer.viewportPosition);
        operation_ = selection_operation_from_modifiers(event.modifiers);
        boxSelecting_ = false;
        begin_interaction();
        return ToolResult::consumed(
            EditorDirtyFlags::None,
            "Selection pointer captured.");
    }

    ToolResult SelectTool::update_pointer_selection(
        ToolContext& context,
        const ToolEvent& event)
    {
        capture_.update_pointer(event.pointer.viewportPosition);

        if (!boxSelecting_ &&
            glm::length(capture_.total_delta()) < dragThresholdPixels_) {
            return ToolResult::ignored();
        }

        boxSelecting_ = true;
        boxShape_.set_rect(selection_rect());

        (void)context.selection_controller()
            .set_hovered_object(SceneNodeId{});
        (void)context.selection_controller()
            .clear_hovered_mesh_component();

        return ToolResult::consumed(
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render,
            "Selection rectangle updated.");
    }

    ToolResult SelectTool::finish_pointer_selection(
        ToolContext& context,
        const ToolEvent& event)
    {
        if (!capture_.matches_button(ToolPointerButton::Primary)) {
            return ToolResult::ignored();
        }

        capture_.update_pointer(event.pointer.viewportPosition);

        ToolResult result =
            boxSelecting_
            ? apply_box_selection(context, event)
            : apply_point_selection(context, event);

        clear_interaction_state();
        finish_interaction();

        return result;
    }

    ToolResult SelectTool::apply_box_selection(
        ToolContext& context,
        const ToolEvent& event)
    {
        if (!context.has_command_services()) {
            return ToolResult::fail(
                "Selection command services are not available.");
        }

        boxShape_.set_rect(selection_rect());
        const SelectionShapeResult hit =
            boxShape_.resolve(
                context,
                event);

        if (is_mesh_granularity(context.selection().granularity())) {
            const SceneNodeId activeMesh =
                context.selection().mesh().active_mesh();

            if (!activeMesh.is_valid()) {
                return ToolResult::ignored();
            }

            auto edit = [
                components = hit.components,
                operation = operation_
            ](kernel::geometry::LEM& mesh,
                SelectionState& selection) {
                (void)mesh;
                MeshSelection& meshSelection = selection.mesh();

                if (operation == SelectionOperation::Replace) {
                    meshSelection.clear_components();
                }

                for (const kernel::geometry::SelectionHit& component :
                    components) {
                    switch (component.type) {
                    case kernel::geometry::LEMElementType::Vertex:
                        if (!component.vertex.is_valid()) {
                            break;
                        }
                        if (operation == SelectionOperation::Subtract) {
                            meshSelection.remove_vertex(component.vertex);
                        }
                        else if (operation == SelectionOperation::Toggle) {
                            meshSelection.toggle_vertex(component.vertex);
                        }
                        else {
                            meshSelection.add_vertex(component.vertex);
                        }
                        break;

                    case kernel::geometry::LEMElementType::Edge:
                        if (!component.edge.is_valid()) {
                            break;
                        }
                        if (operation == SelectionOperation::Subtract) {
                            meshSelection.remove_edge(component.edge);
                        }
                        else if (operation == SelectionOperation::Toggle) {
                            meshSelection.toggle_edge(component.edge);
                        }
                        else {
                            meshSelection.add_edge(component.edge);
                        }
                        break;

                    case kernel::geometry::LEMElementType::Loop:
                        break;

                    case kernel::geometry::LEMElementType::Face:
                        if (!component.face.is_valid()) {
                            break;
                        }
                        if (operation == SelectionOperation::Subtract) {
                            meshSelection.remove_face(component.face);
                        }
                        else if (operation == SelectionOperation::Toggle) {
                            meshSelection.toggle_face(component.face);
                        }
                        else {
                            meshSelection.add_face(component.face);
                        }
                        break;
                    }
                }

                selection.mark_dirty();
                return true;
            };

            return from_command_result(
                context.execute_command(
                    std::make_unique<EditMeshSelectionCommand>(
                        activeMesh,
                        std::move(edit),
                        "Box Select Mesh Components")),
                "Mesh components box selected.");
        }

        if (hit.objects.empty() &&
            operation_ != SelectionOperation::Replace) {
            return ToolResult::ignored();
        }

        return from_command_result(
            context.execute_command(
                std::make_unique<SetObjectSelectionCommand>(
                    hit.objects,
                    operation_)),
            "Objects box selected.");
    }

    void SelectTool::clear_interaction_state()
    {
        capture_.clear();
        boxSelecting_ = false;
        operation_ = SelectionOperation::Replace;
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
