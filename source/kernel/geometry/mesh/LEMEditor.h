/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/render/NormalBuilder.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <glm/glm.hpp>

#include <vector>

namespace locus::kernel::geometry {

/**
 * @brief Editing helper that mutates a LEM while recording a change diff.
 */
class LEMEditor {
public:
    /**
     * @brief Creates an editor bound to an editable mesh.
     *
     * @param mesh Mesh that receives all mutations.
     */
    explicit LEMEditor(LEM& mesh)
        : mesh_(mesh)
    {
    }

    /**
     * @brief Returns the edited mesh.
     *
     * @return Mutable mesh reference.
     */
    [[nodiscard]] LEM& mesh()
    {
        return mesh_;
    }

    /**
     * @brief Returns the edited mesh.
     *
     * @return Read-only mesh reference.
     */
    [[nodiscard]] const LEM& mesh() const
    {
        return mesh_;
    }

    /**
     * @brief Returns the accumulated edit diff.
     *
     * @return Read-only diff reference.
     */
    [[nodiscard]] const LEMDiff& diff() const
    {
        return diff_;
    }

    /**
     * @brief Moves the accumulated diff out of the editor and clears it.
     *
     * @return Diff recorded since the last clear or take.
     */
    [[nodiscard]] LEMDiff take_diff()
    {
        LEMDiff result = diff_;
        diff_.clear();
        return result;
    }

    /**
     * @brief Clears the accumulated diff without changing the mesh.
     */
    void clear_diff()
    {
        diff_.clear();
    }

    /**
     * @brief Adds a vertex and records the created element.
     *
     * @param position Vertex position in object space.
     * @return Handle referencing the created vertex.
     */
    VertexHandle add_vertex(const glm::vec3& position)
    {
        VertexHandle handle = mesh_.add_vertex(position);

        if (mesh_.is_valid(handle)) {
            diff_.record(LEMChangeType::VertexAdded, handle);
        }

        return handle;
    }

    /**
     * @brief Finds or creates an edge and records topology changes when inserted.
     *
     * @param vertexA First endpoint vertex.
     * @param vertexB Second endpoint vertex.
     * @return Handle referencing the existing or created edge.
     */
    EdgeHandle find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB)
    {
        const std::size_t edgeCount = mesh_.edge_count();
        EdgeHandle handle = mesh_.find_or_create_edge(vertexA, vertexB);

        if (mesh_.is_valid(handle) && mesh_.edge_count() > edgeCount) {
            diff_.record(LEMChangeType::EdgeAdded, handle);
            diff_.record(LEMChangeType::VertexModified, vertexA);
            diff_.record(LEMChangeType::VertexModified, vertexB);
        }

        return handle;
    }

    /**
     * @brief Adds a polygonal face and records all new topology elements.
     *
     * @param vertices Ordered face vertices.
     * @return Handle referencing the created face, or an invalid handle on failure.
     * @note The vertex order defines the face winding and normal direction.
     */
    FaceHandle add_face(const std::vector<VertexHandle>& vertices)
    {
        const std::size_t edgeCount = mesh_.edge_count();
        const std::size_t loopCount = mesh_.loop_count();
        const std::size_t faceCount = mesh_.face_count();

        FaceHandle handle = mesh_.add_face(vertices);

        if (!mesh_.is_valid(handle)) {
            return {};
        }

        for (std::size_t index = edgeCount; index < mesh_.edge_count(); ++index) {
            diff_.record(LEMChangeType::EdgeAdded, EdgeHandle(static_cast<IdValue>(index)));
        }

        for (std::size_t index = loopCount; index < mesh_.loop_count(); ++index) {
            diff_.record(LEMChangeType::LoopAdded, LoopHandle(static_cast<IdValue>(index)));
        }

        for (std::size_t index = faceCount; index < mesh_.face_count(); ++index) {
            diff_.record(LEMChangeType::FaceAdded, FaceHandle(static_cast<IdValue>(index)));
        }

        for (VertexHandle vertexHandle : vertices) {
            if (mesh_.is_valid(vertexHandle)) {
                diff_.record(LEMChangeType::VertexModified, vertexHandle);
            }
        }

        return handle;
    }

    /**
     * @brief Sets a vertex position and updates adjacent face normals.
     *
     * @param handle Vertex to modify.
     * @param position New object-space position.
     * @return True when the vertex exists and the request was accepted.
     */
    bool set_vertex_position(VertexHandle handle, const glm::vec3& position)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);

        if (vertex.position == position) {
            return true;
        }

        vertex.position = position;
        diff_.record(LEMChangeType::VertexModified, handle);

        for (FaceHandle faceHandle : TopologyTraversal::vertex_faces(mesh_, handle)) {
            if (mesh_.is_valid(faceHandle)) {
                mesh_.face(faceHandle).normal = NormalBuilder::face_normal(mesh_, faceHandle);
                diff_.record(LEMChangeType::NormalsChanged, faceHandle);
            }
        }

        return true;
    }

    /**
     * @brief Translates a single vertex by an object-space offset.
     *
     * @param handle Vertex to translate.
     * @param offset Translation offset.
     * @return True when the vertex exists and the request was accepted.
     */
    bool translate_vertex(VertexHandle handle, const glm::vec3& offset)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        return set_vertex_position(handle, mesh_.vertex(handle).position + offset);
    }

    /**
     * @brief Translates multiple vertices by the same object-space offset.
     *
     * @param vertices Vertices to translate.
     * @param offset Translation offset.
     * @return Number of vertices accepted by the edit.
     */
    std::size_t translate_vertices(const std::vector<VertexHandle>& vertices, const glm::vec3& offset)
    {
        std::size_t count = 0;

        for (VertexHandle handle : vertices) {
            if (translate_vertex(handle, offset)) {
                ++count;
            }
        }

        return count;
    }

    /**
     * @brief Changes a vertex selection flag.
     *
     * @param handle Vertex to modify.
     * @param selected New selection state.
     * @return True when the vertex exists and the request was accepted.
     */
    bool set_selected(VertexHandle handle, bool selected)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);

        if (vertex.selected == selected) {
            return true;
        }

        vertex.selected = selected;
        diff_.record(LEMChangeType::SelectionChanged, handle);
        return true;
    }

    /**
     * @brief Changes an edge selection flag.
     *
     * @param handle Edge to modify.
     * @param selected New selection state.
     * @return True when the edge exists and the request was accepted.
     */
    bool set_selected(EdgeHandle handle, bool selected)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Edge& edge = mesh_.edge(handle);

        if (edge.selected == selected) {
            return true;
        }

        edge.selected = selected;
        diff_.record(LEMChangeType::SelectionChanged, handle);
        return true;
    }

    /**
     * @brief Changes a face selection flag.
     *
     * @param handle Face to modify.
     * @param selected New selection state.
     * @return True when the face exists and the request was accepted.
     */
    bool set_selected(FaceHandle handle, bool selected)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Face& face = mesh_.face(handle);

        if (face.selected == selected) {
            return true;
        }

        face.selected = selected;
        diff_.record(LEMChangeType::SelectionChanged, handle);
        return true;
    }

    /**
     * @brief Clears selection flags on all active vertices, edges, and faces.
     */
    void clear_selection()
    {
        for (VertexHandle handle : TopologyTraversal::vertices(mesh_)) {
            set_selected(handle, false);
        }

        for (EdgeHandle handle : TopologyTraversal::edges(mesh_)) {
            set_selected(handle, false);
        }

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            set_selected(handle, false);
        }
    }

    /**
     * @brief Changes a vertex visibility flag.
     *
     * @param handle Vertex to modify.
     * @param hidden New hidden state.
     * @return True when the vertex exists and the request was accepted.
     */
    bool set_hidden(VertexHandle handle, bool hidden)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Vertex& vertex = mesh_.vertex(handle);

        if (vertex.hidden == hidden) {
            return true;
        }

        vertex.hidden = hidden;
        diff_.record(LEMChangeType::VisibilityChanged, handle);
        return true;
    }

    /**
     * @brief Changes an edge visibility flag.
     *
     * @param handle Edge to modify.
     * @param hidden New hidden state.
     * @return True when the edge exists and the request was accepted.
     */
    bool set_hidden(EdgeHandle handle, bool hidden)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Edge& edge = mesh_.edge(handle);

        if (edge.hidden == hidden) {
            return true;
        }

        edge.hidden = hidden;
        diff_.record(LEMChangeType::VisibilityChanged, handle);
        return true;
    }

    /**
     * @brief Changes a face visibility flag.
     *
     * @param handle Face to modify.
     * @param hidden New hidden state.
     * @return True when the face exists and the request was accepted.
     */
    bool set_hidden(FaceHandle handle, bool hidden)
    {
        if (!mesh_.is_valid(handle)) {
            return false;
        }

        Face& face = mesh_.face(handle);

        if (face.hidden == hidden) {
            return true;
        }

        face.hidden = hidden;
        diff_.record(LEMChangeType::VisibilityChanged, handle);
        return true;
    }

    /**
     * @brief Clears hidden flags on all active vertices, edges, and faces.
     */
    void clear_visibility()
    {
        for (VertexHandle handle : TopologyTraversal::vertices(mesh_)) {
            set_hidden(handle, false);
        }

        for (EdgeHandle handle : TopologyTraversal::edges(mesh_)) {
            set_hidden(handle, false);
        }

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            set_hidden(handle, false);
        }
    }

    /**
     * @brief Recomputes all active face normals and records normal changes.
     */
    void rebuild_face_normals()
    {
        NormalBuilder::rebuild_face_normals(mesh_);

        for (FaceHandle handle : TopologyTraversal::faces(mesh_)) {
            diff_.record(LEMChangeType::NormalsChanged, handle);
        }
    }

    /**
     * @brief Clears the mesh and records a whole-mesh clear event.
     */
    void clear()
    {
        mesh_.clear();
        diff_.record(LEMChangeType::MeshCleared, LEMElementType::Vertex, Id{});
    }

private:
    LEM& mesh_;
    LEMDiff diff_{};
};

}
