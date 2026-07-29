/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/selection/SelectionController.h"

namespace locus::editor {

    SelectionController::SelectionController(EditorScene& scene, SelectionState& state)
        : scene_(&scene)
        , state_(&state)
    {
    }

    bool SelectionController::select_object(SceneNodeId id)
    {
        if (!is_valid_selectable_object(id)) {
            return false;
        }

        enter_object_mode();
        state_->objects().set(id);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::add_object(SceneNodeId id)
    {
        if (!is_valid_selectable_object(id)) {
            return false;
        }

        enter_object_mode();
        const bool added = state_->objects().add(id);

        if (added) {
            state_->mark_dirty();
        }

        return added;
    }

    bool SelectionController::remove_object(SceneNodeId id)
    {
        const bool removed = state_->objects().remove(id);

        if (removed) {
            state_->mark_dirty();
        }

        return removed;
    }

    bool SelectionController::toggle_object(SceneNodeId id)
    {
        if (!is_valid_selectable_object(id)) {
            return false;
        }

        enter_object_mode();
        const bool selected = state_->objects().toggle(id);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::set_active_object(SceneNodeId id)
    {
        if (!is_valid_selectable_object(id)) {
            return false;
        }

        enter_object_mode();
        state_->objects().set_active(id);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::set_hovered_object(SceneNodeId id)
    {
        if (id.is_valid() && !is_valid_selectable_object(id)) {
            return false;
        }

        state_->objects().set_hovered(id);
        state_->mark_dirty();
        return true;
    }

    void SelectionController::clear_objects()
    {
        state_->objects().clear();
        state_->mark_dirty();
    }

    bool SelectionController::set_active_mesh(SceneNodeId id)
    {
        if (!is_valid_mesh(id)) {
            return false;
        }

        state_->mesh().set_active_mesh(id);
        state_->objects().set_active(id);
        state_->set_scope(SelectionScope::ActiveMesh);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::select_vertex(kernel::geometry::VertexHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        enter_mesh_mode(SelectionGranularity::Vertex);
        state_->mesh().set_vertex(handle);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::select_edge(kernel::geometry::EdgeHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        enter_mesh_mode(SelectionGranularity::Edge);
        state_->mesh().set_edge(handle);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::select_loop(kernel::geometry::LoopHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        enter_mesh_mode(SelectionGranularity::Loop);
        state_->mesh().set_loop(handle);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::select_face(kernel::geometry::FaceHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        enter_mesh_mode(SelectionGranularity::Face);
        state_->mesh().set_face(handle);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::toggle_vertex(kernel::geometry::VertexHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        enter_mesh_mode(SelectionGranularity::Vertex);
        const bool selected = state_->mesh().toggle_vertex(handle);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::toggle_edge(kernel::geometry::EdgeHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        enter_mesh_mode(SelectionGranularity::Edge);
        const bool selected = state_->mesh().toggle_edge(handle);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::toggle_loop(kernel::geometry::LoopHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        enter_mesh_mode(SelectionGranularity::Loop);
        const bool selected = state_->mesh().toggle_loop(handle);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::toggle_face(kernel::geometry::FaceHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        enter_mesh_mode(SelectionGranularity::Face);
        const bool selected = state_->mesh().toggle_face(handle);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::set_hovered_mesh_component(
        const kernel::geometry::SelectionHit& hit)
    {
        kernel::geometry::VertexHandle vertex{};
        kernel::geometry::EdgeHandle edge{};
        kernel::geometry::LoopHandle loop{};
        kernel::geometry::FaceHandle face{};

        if (hit.is_vertex()) {
            vertex = hit.vertex;
        }
        else if (hit.is_edge()) {
            edge = hit.edge;
        }
        else if (hit.is_loop()) {
            loop = hit.loop;
        }
        else if (hit.is_face()) {
            face = hit.face;
        }

        MeshSelection& mesh = state_->mesh();
        const bool changed =
            mesh.hovered_vertex() != vertex ||
            mesh.hovered_edge() != edge ||
            mesh.hovered_loop() != loop ||
            mesh.hovered_face() != face;

        if (!changed) {
            return false;
        }

        mesh.set_hovered_vertex(vertex);
        mesh.set_hovered_edge(edge);
        mesh.set_hovered_loop(loop);
        mesh.set_hovered_face(face);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::clear_hovered_mesh_component()
    {
        return set_hovered_mesh_component(
            kernel::geometry::SelectionHit::miss());
    }

    void SelectionController::clear_mesh_components()
    {
        state_->mesh().clear_components();
        state_->mark_dirty();
    }

    void SelectionController::set_granularity(SelectionGranularity granularity)
    {
        state_->set_granularity(granularity);
    }

    bool SelectionController::is_valid_selectable_object(SceneNodeId id) const
    {
        const SceneNode* node = scene_->find_node(id);
        return node && node->is_selectable();
    }

    bool SelectionController::is_valid_mesh(SceneNodeId id) const
    {
        const MeshNode* node = scene_->find_mesh(id);
        return node && node->is_selectable();
    }

    void SelectionController::enter_object_mode()
    {
        state_->set_scope(SelectionScope::Scene);
        state_->set_granularity(SelectionGranularity::Object);
    }

    void SelectionController::enter_mesh_mode(SelectionGranularity granularity)
    {
        state_->set_scope(SelectionScope::ActiveMesh);
        state_->set_granularity(granularity);
    }

}
