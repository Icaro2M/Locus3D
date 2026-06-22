/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/editing/TopologyEditor.h"

#include "kernel/common/Id.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/editing/topology/TopologyCollapse.h"
#include "kernel/geometry/mesh/editing/topology/TopologyCreation.h"
#include "kernel/geometry/mesh/editing/topology/TopologyFlip.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRelink.h"
#include "kernel/geometry/mesh/editing/topology/TopologyRemoval.h"
#include "kernel/geometry/mesh/editing/topology/TopologySplit.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

namespace locus::kernel::geometry {

    TopologyEditor::TopologyEditor(LEM& mesh, LEMDiff& diff)
        : mesh_(mesh)
        , diff_(diff)
    {
    }

    LEM& TopologyEditor::mesh()
    {
        return mesh_;
    }

    const LEM& TopologyEditor::mesh() const
    {
        return mesh_;
    }

    VertexHandle TopologyEditor::add_vertex(const glm::vec3& position)
    {
        return editing::topology::add_vertex(mesh_, diff_, position);
    }

    EdgeHandle TopologyEditor::find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB)
    {
        return editing::topology::find_or_create_edge(mesh_, diff_, vertexA, vertexB);
    }

    FaceHandle TopologyEditor::add_face(const std::vector<VertexHandle>& vertices)
    {
        return editing::topology::add_face(mesh_, diff_, vertices);
    }

    bool TopologyEditor::remove_face(FaceHandle face)
    {
        return editing::topology::remove_face(mesh_, diff_, face);
    }

    bool TopologyEditor::remove_edge_if_loose(EdgeHandle edge)
    {
        return editing::topology::remove_edge_if_loose(mesh_, diff_, edge);
    }

    bool TopologyEditor::remove_vertex_if_loose(VertexHandle vertex)
    {
        return editing::topology::remove_vertex_if_loose(mesh_, diff_, vertex);
    }

    bool TopologyEditor::kill_face_only(FaceHandle face)
    {
        return editing::topology::kill_face_only(mesh_, diff_, face);
    }

    bool TopologyEditor::kill_edge_only(EdgeHandle edge)
    {
        return editing::topology::kill_edge_only(mesh_, diff_, edge);
    }

    bool TopologyEditor::kill_loop(LoopHandle loop)
    {
        return editing::topology::kill_loop(mesh_, diff_, loop);
    }

    std::optional<VertexHandle> TopologyEditor::split_edge(EdgeHandle edge)
    {
        return editing::topology::split_edge(mesh_, diff_, edge);
    }

    std::optional<VertexHandle> TopologyEditor::split_edge_at_param(EdgeHandle edge, float t)
    {
        return editing::topology::split_edge_at_param(mesh_, diff_, edge, t);
    }

    std::optional<EdgeHandle> TopologyEditor::split_face(
        FaceHandle face,
        VertexHandle vertexA,
        VertexHandle vertexB)
    {
        return editing::topology::split_face(mesh_, diff_, face, vertexA, vertexB);
    }

    bool TopologyEditor::collapse_edge(EdgeHandle edge)
    {
        return editing::topology::collapse_edge(mesh_, diff_, edge);
    }

    bool TopologyEditor::merge_vertices(VertexHandle sourceVertex, VertexHandle targetVertex)
    {
        return editing::topology::merge_vertices(
            mesh_,
            diff_,
            sourceVertex,
            targetVertex);
    }

    bool TopologyEditor::merge_vertices_at_position(
        VertexHandle sourceVertex,
        VertexHandle targetVertex,
        const glm::vec3& position)
    {
        return editing::topology::merge_vertices_at_position(
            mesh_,
            diff_,
            sourceVertex,
            targetVertex,
            position);
    }

    std::size_t TopologyEditor::merge_vertices_by_distance(float distance)
    {
        return editing::topology::merge_vertices_by_distance(
            mesh_,
            diff_,
            distance);
    }

    std::size_t TopologyEditor::weld_vertices(
        const std::vector<VertexHandle>& vertices,
        float distance)
    {
        return editing::topology::weld_vertices(
            mesh_,
            diff_,
            vertices,
            distance);
    }

    bool TopologyEditor::dissolve_edge(EdgeHandle edge)
    {
        return editing::topology::dissolve_edge(mesh_, diff_, edge);
    }

    bool TopologyEditor::dissolve_vertex(VertexHandle vertex)
    {
        return editing::topology::dissolve_vertex(mesh_, diff_, vertex);
    }

    bool TopologyEditor::dissolve_face(FaceHandle face)
    {
        return editing::topology::dissolve_face(mesh_, diff_, face);
    }

    bool TopologyEditor::flip_face(FaceHandle face)
    {
        return editing::topology::flip_face(mesh_, diff_, face);
    }

    std::size_t TopologyEditor::flip_all_faces()
    {
        return editing::topology::flip_all_faces(mesh_, diff_);
    }

    bool TopologyEditor::flip_edge(EdgeHandle edge)
    {
        return editing::topology::flip_edge(mesh_, diff_, edge);
    }

    void TopologyEditor::rebuild_face_normals()
    {
        NormalBuilder::rebuild_face_normals(mesh_);

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            diff_.record(LEMChangeType::NormalsChanged, handle);
        }
    }

    void TopologyEditor::clear()
    {
        mesh_.clear();
        diff_.record(LEMChangeType::MeshCleared, LEMElementType::Vertex, Id{});
    }

    bool TopologyEditor::remove_loop_from_radial(LoopHandle loop)
    {
        return editing::topology::remove_loop_from_radial(mesh_, diff_, loop);
    }

    bool TopologyEditor::insert_loop_into_radial(LoopHandle loop, EdgeHandle edge)
    {
        return editing::topology::insert_loop_into_radial(mesh_, diff_, loop, edge);
    }

    void TopologyEditor::refresh_edge_entry_loop(EdgeHandle edge)
    {
        editing::topology::refresh_edge_entry_loop(mesh_, diff_, edge);
    }

    void TopologyEditor::refresh_vertex_incident_edge(VertexHandle vertex)
    {
        editing::topology::refresh_vertex_incident_edge(mesh_, diff_, vertex);
    }

    bool TopologyEditor::update_loop_vertex(LoopHandle loop, VertexHandle vertex)
    {
        return editing::topology::update_loop_vertex(mesh_, diff_, loop, vertex);
    }

    bool TopologyEditor::update_loop_edge(LoopHandle loop, EdgeHandle edge)
    {
        return editing::topology::update_loop_edge(mesh_, diff_, loop, edge);
    }

    bool TopologyEditor::replace_vertex_in_face(
        FaceHandle face,
        VertexHandle oldVertex,
        VertexHandle newVertex)
    {
        return editing::topology::replace_vertex_in_face(mesh_, diff_, face, oldVertex, newVertex);
    }

}