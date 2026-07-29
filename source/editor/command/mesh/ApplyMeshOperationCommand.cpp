/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/command/mesh/ApplyMeshOperationCommand.h"

#include "editor/scene/EditorScene.h"
#include "editor/scene/MeshNode.h"

#include <vector>
#include <utility>

namespace locus::editor {

    namespace {

        void prune_invalid_mesh_selection(
            SelectionState& selection,
            const MeshNode& node)
        {
            MeshSelection& meshSelection = selection.mesh();
            const kernel::geometry::LEM& mesh = node.mesh();

            const std::vector<kernel::geometry::VertexHandle> vertices =
                meshSelection.vertices().items();
            const std::vector<kernel::geometry::EdgeHandle> edges =
                meshSelection.edges().items();
            const std::vector<kernel::geometry::LoopHandle> loops =
                meshSelection.loops().items();
            const std::vector<kernel::geometry::FaceHandle> faces =
                meshSelection.faces().items();

            const kernel::geometry::VertexHandle hoveredVertex =
                meshSelection.hovered_vertex();
            const kernel::geometry::EdgeHandle hoveredEdge =
                meshSelection.hovered_edge();
            const kernel::geometry::LoopHandle hoveredLoop =
                meshSelection.hovered_loop();
            const kernel::geometry::FaceHandle hoveredFace =
                meshSelection.hovered_face();

            meshSelection.clear_components();

            for (const kernel::geometry::VertexHandle handle : vertices) {
                if (mesh.is_valid(handle)) {
                    meshSelection.add_vertex(handle);
                }
            }

            for (const kernel::geometry::EdgeHandle handle : edges) {
                if (mesh.is_valid(handle)) {
                    meshSelection.add_edge(handle);
                }
            }

            for (const kernel::geometry::LoopHandle handle : loops) {
                if (mesh.is_valid(handle)) {
                    meshSelection.add_loop(handle);
                }
            }

            for (const kernel::geometry::FaceHandle handle : faces) {
                if (mesh.is_valid(handle)) {
                    meshSelection.add_face(handle);
                }
            }

            if (mesh.is_valid(hoveredVertex)) {
                meshSelection.set_hovered_vertex(hoveredVertex);
            }

            if (mesh.is_valid(hoveredEdge)) {
                meshSelection.set_hovered_edge(hoveredEdge);
            }

            if (mesh.is_valid(hoveredLoop)) {
                meshSelection.set_hovered_loop(hoveredLoop);
            }

            if (mesh.is_valid(hoveredFace)) {
                meshSelection.set_hovered_face(hoveredFace);
            }
        }

    } // namespace

    ApplyMeshOperationCommand::ApplyMeshOperationCommand(
        SceneNodeId meshNode,
        MeshOperation operation,
        std::string label)
        : meshNode_(meshNode)
        , operation_(std::move(operation))
        , label_(std::move(label))
    {
        if (label_.empty()) {
            label_ = "Apply Mesh Operation";
        }
    }

    std::string_view ApplyMeshOperationCommand::name() const
    {
        return label_;
    }

    CommandResult ApplyMeshOperationCommand::execute(CommandContext& context)
    {
        if (executed_) {
            return redo(context);
        }

        if (meshNode_.is_invalid()) {
            return CommandResult::fail("Cannot apply mesh operation to an invalid node.");
        }

        if (!operation_) {
            return CommandResult::fail("Cannot apply mesh operation without an operation callback.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot apply mesh operation to a missing mesh node.");
        }

        previousSnapshot_.capture(*node, context.selection());

        kernel::geometry::LEMEditor editor(node->mesh());
        const bool success = operation_(editor);

        if (!success) {
            previousSnapshot_.restore(*node, context.selection());
            return CommandResult::fail("Mesh operation failed.");
        }

        node->bump_mesh_revision();
        context.selection().mesh().set_active_mesh(meshNode_);
        prune_invalid_mesh_selection(context.selection(), *node);
        context.selection().mark_dirty();

        node->mark_dirty(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking);

        nextSnapshot_.capture(*node, context.selection());
        executed_ = true;

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh operation applied.");
    }

    CommandResult ApplyMeshOperationCommand::undo(CommandContext& context)
    {
        if (!executed_ || !previousSnapshot_.is_valid()) {
            return CommandResult::fail("Cannot undo mesh operation before execution.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot undo mesh operation because the mesh node is missing.");
        }

        previousSnapshot_.restore(*node, context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh operation undone.");
    }

    CommandResult ApplyMeshOperationCommand::redo(CommandContext& context)
    {
        if (!executed_ || !nextSnapshot_.is_valid()) {
            return CommandResult::fail("Cannot redo mesh operation before execution.");
        }

        MeshNode* node = find_target(context);
        if (!node) {
            return CommandResult::fail("Cannot redo mesh operation because the mesh node is missing.");
        }

        nextSnapshot_.restore(*node, context.selection());

        return CommandResult::ok(
            EditorDirtyFlags::Mesh |
            EditorDirtyFlags::Selection |
            EditorDirtyFlags::Render |
            EditorDirtyFlags::Picking,
            "Mesh operation applied.");
    }

    MeshNode* ApplyMeshOperationCommand::find_target(CommandContext& context) const
    {
        return context.scene().find_mesh(meshNode_);
    }

} // namespace locus::editor    
