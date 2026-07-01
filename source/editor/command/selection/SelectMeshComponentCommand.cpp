/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/selection/SelectMeshComponentCommand.h"

#include "editor/scene/MeshNode.h"
#include "editor/selection/MeshSelection.h"
#include "editor/selection/SelectionController.h"

#include <string>

namespace locus::editor {

    SelectMeshComponentCommand::SelectMeshComponentCommand(kernel::geometry::VertexHandle handle)
        : component_(SelectionGranularity::Vertex)
        , vertex_(handle)
    {
    }

    SelectMeshComponentCommand::SelectMeshComponentCommand(kernel::geometry::EdgeHandle handle)
        : component_(SelectionGranularity::Edge)
        , edge_(handle)
    {
    }

    SelectMeshComponentCommand::SelectMeshComponentCommand(kernel::geometry::LoopHandle handle)
        : component_(SelectionGranularity::Loop)
        , loop_(handle)
    {
    }

    SelectMeshComponentCommand::SelectMeshComponentCommand(kernel::geometry::FaceHandle handle)
        : component_(SelectionGranularity::Face)
        , face_(handle)
    {
    }

    std::string_view SelectMeshComponentCommand::name() const
    {
        return "Select Mesh Component";
    }

    CommandResult SelectMeshComponentCommand::execute(CommandContext& context)
    {
        if (!has_valid_handle()) {
            return CommandResult::fail("Cannot select an invalid mesh component.");
        }

        const SceneNodeId activeMesh = context.selection().mesh().active_mesh();
        if (activeMesh.is_invalid() || !context.scene().find_mesh(activeMesh)) {
            return CommandResult::fail("Cannot select a mesh component without a valid active mesh.");
        }

        previousSelection_.capture(context.selection());

        CommandResult result = select_component(context);
        if (!result.success) {
            previousSelection_.restore(context.selection());
            return result;
        }

        executed_ = true;
        return result;
    }

    CommandResult SelectMeshComponentCommand::undo(CommandContext& context)
    {
        if (!executed_ || !previousSelection_.is_valid()) {
            return CommandResult::fail("Cannot undo mesh component selection without a previous selection snapshot.");
        }

        previousSelection_.restore(context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            "Mesh component selection restored.");
    }

    bool SelectMeshComponentCommand::has_valid_handle() const
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

    CommandResult SelectMeshComponentCommand::select_component(CommandContext& context)
    {
        context.selection_controller().clear_mesh_components();

        bool selected = false;

        switch (component_) {
        case SelectionGranularity::Vertex:
            selected = context.selection_controller().select_vertex(vertex_);
            break;
        case SelectionGranularity::Edge:
            selected = context.selection_controller().select_edge(edge_);
            break;
        case SelectionGranularity::Loop:
            selected = context.selection_controller().select_loop(loop_);
            break;
        case SelectionGranularity::Face:
            selected = context.selection_controller().select_face(face_);
            break;
        case SelectionGranularity::Object:
            return CommandResult::fail("Cannot select an object as a mesh component.");
        }

        if (!selected) {
            return CommandResult::fail("Mesh component selection failed.");
        }

        return CommandResult::ok(
            EditorDirtyFlags::Selection | EditorDirtyFlags::Render | EditorDirtyFlags::Picking,
            std::string(component_name()) + " selected.");
    }

    const char* SelectMeshComponentCommand::component_name() const
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