/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/ToggleMeshComponentSelectionCommand.h"

#include "editor/scene/MeshNode.h"
#include "editor/selection/MeshSelection.h"
#include "editor/selection/SelectionController.h"

#include <string>

namespace locus::editor {

    ToggleMeshComponentSelectionCommand::ToggleMeshComponentSelectionCommand(kernel::geometry::VertexHandle handle)
        : component_(SelectionGranularity::Vertex)
        , vertex_(handle)
    {
    }

    ToggleMeshComponentSelectionCommand::ToggleMeshComponentSelectionCommand(kernel::geometry::EdgeHandle handle)
        : component_(SelectionGranularity::Edge)
        , edge_(handle)
    {
    }

    ToggleMeshComponentSelectionCommand::ToggleMeshComponentSelectionCommand(kernel::geometry::LoopHandle handle)
        : component_(SelectionGranularity::Loop)
        , loop_(handle)
    {
    }

    ToggleMeshComponentSelectionCommand::ToggleMeshComponentSelectionCommand(kernel::geometry::FaceHandle handle)
        : component_(SelectionGranularity::Face)
        , face_(handle)
    {
    }

    std::string_view ToggleMeshComponentSelectionCommand::name() const
    {
        return "Toggle Mesh Component Selection";
    }

    CommandResult ToggleMeshComponentSelectionCommand::execute(CommandContext& context)
    {
        if (!has_valid_handle()) {
            return CommandResult::fail("Cannot toggle an invalid mesh component.");
        }

        const SceneNodeId activeMesh = context.selection().mesh().active_mesh();
        if (activeMesh.is_invalid() || !context.scene().find_mesh(activeMesh)) {
            return CommandResult::fail("Cannot toggle a mesh component without a valid active mesh.");
        }

        previousSelection_.capture(context.selection());

        CommandResult result = toggle_component(context);
        if (!result.success) {
            return result;
        }

        executed_ = true;
        return result;
    }

    CommandResult ToggleMeshComponentSelectionCommand::undo(CommandContext& context)
    {
        if (!executed_ || !previousSelection_.is_valid()) {
            return CommandResult::fail("Cannot undo mesh component toggle without a previous selection snapshot.");
        }

        previousSelection_.restore(context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            "Mesh component selection restored.");
    }

    bool ToggleMeshComponentSelectionCommand::has_valid_handle() const
    {
        switch (component_) {
        case SelectionGranularity::Vertex:
            return vertex_.is_valid();
        case SelectionGranularity::Edge:
            return edge_.is_valid();
        case SelectionGranularity::Loop:
            return loop_.is_valid();
        case SelectionGranularity::Face:
            return face_.is_valid();
        case SelectionGranularity::Object:
            return false;
        }

        return false;
    }

    CommandResult ToggleMeshComponentSelectionCommand::toggle_component(CommandContext& context)
    {
        switch (component_) {
        case SelectionGranularity::Vertex:
            selectedAfterExecute_ = context.selection_controller().toggle_vertex(vertex_);
            break;
        case SelectionGranularity::Edge:
            selectedAfterExecute_ = context.selection_controller().toggle_edge(edge_);
            break;
        case SelectionGranularity::Loop:
            selectedAfterExecute_ = context.selection_controller().toggle_loop(loop_);
            break;
        case SelectionGranularity::Face:
            selectedAfterExecute_ = context.selection_controller().toggle_face(face_);
            break;
        case SelectionGranularity::Object:
            return CommandResult::fail("Cannot toggle an object as a mesh component.");
        }

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            selectedAfterExecute_
            ? std::string(component_name()) + " added to selection."
            : std::string(component_name()) + " removed from selection.");
    }

    const char* ToggleMeshComponentSelectionCommand::component_name() const
    {
        switch (component_) {
        case SelectionGranularity::Vertex:
            return "Vertex";
        case SelectionGranularity::Edge:
            return "Edge";
        case SelectionGranularity::Loop:
            return "Loop";
        case SelectionGranularity::Face:
            return "Face";
        case SelectionGranularity::Object:
            return "Object";
        }

        return "Unknown";
    }

} // namespace locus::editor