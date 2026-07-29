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

        enter_scene_context();
        state_->objects().set(id);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::add_object(SceneNodeId id)
    {
        if (!is_valid_selectable_object(id)) {
            return false;
        }

        enter_scene_context();
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

        enter_scene_context();
        const bool selected = state_->objects().toggle(id);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::set_active_object(SceneNodeId id)
    {
        if (!is_valid_selectable_object(id)) {
            return false;
        }

        enter_scene_context();
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
        return enter_mesh_context(
            id,
            is_mesh_granularity(state_->granularity())
            ? state_->granularity()
            : SelectionGranularity::Face);
    }

    void SelectionController::enter_scene_context()
    {
        state_->mesh().clear();
        state_->set_scope(SelectionScope::Scene);
        state_->set_granularity(SelectionGranularity::Object);
        state_->mark_dirty();
    }

    bool SelectionController::enter_mesh_context(
        SceneNodeId id,
        SelectionGranularity granularity)
    {
        if (!is_valid_mesh(id) || !is_mesh_granularity(granularity)) {
            return false;
        }

        state_->mesh().set_active_mesh(id);
        state_->objects().set_active(id);
        state_->set_scope(SelectionScope::ActiveMesh);
        state_->set_granularity(granularity);
        state_->mark_dirty();
        return true;
    }

    void SelectionController::leave_mesh_context()
    {
        enter_scene_context();
    }

    bool SelectionController::select_vertex(kernel::geometry::VertexHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        set_mesh_granularity(SelectionGranularity::Vertex);
        state_->mesh().set_vertex(handle);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::select_edge(kernel::geometry::EdgeHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        set_mesh_granularity(SelectionGranularity::Edge);
        state_->mesh().set_edge(handle);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::select_loop(kernel::geometry::LoopHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        set_mesh_granularity(SelectionGranularity::Loop);
        state_->mesh().set_loop(handle);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::select_face(kernel::geometry::FaceHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        set_mesh_granularity(SelectionGranularity::Face);
        state_->mesh().set_face(handle);
        state_->mark_dirty();
        return true;
    }

    bool SelectionController::toggle_vertex(kernel::geometry::VertexHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        set_mesh_granularity(SelectionGranularity::Vertex);
        const bool selected = state_->mesh().toggle_vertex(handle);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::toggle_edge(kernel::geometry::EdgeHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        set_mesh_granularity(SelectionGranularity::Edge);
        const bool selected = state_->mesh().toggle_edge(handle);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::toggle_loop(kernel::geometry::LoopHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        set_mesh_granularity(SelectionGranularity::Loop);
        const bool selected = state_->mesh().toggle_loop(handle);
        state_->mark_dirty();
        return selected;
    }

    bool SelectionController::toggle_face(kernel::geometry::FaceHandle handle)
    {
        if (handle.is_invalid()) {
            return false;
        }

        set_mesh_granularity(SelectionGranularity::Face);
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
        if (granularity == SelectionGranularity::Object) {
            enter_scene_context();
            return;
        }

        const SceneNodeId mesh =
            state_->mesh().active_mesh().is_valid()
            ? state_->mesh().active_mesh()
            : active_object_mesh();

        if (mesh.is_valid()) {
            (void)enter_mesh_context(mesh, granularity);
            return;
        }

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

    SceneNodeId SelectionController::active_object_mesh() const
    {
        const SceneNodeId active = state_->objects().active();
        return is_valid_mesh(active) ? active : SceneNodeId{};
    }

    void SelectionController::set_mesh_granularity(
        SelectionGranularity granularity)
    {
        state_->set_scope(SelectionScope::ActiveMesh);
        state_->set_granularity(granularity);
    }

}
