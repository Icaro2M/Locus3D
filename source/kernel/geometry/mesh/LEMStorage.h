/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Face.h"
#include "kernel/geometry/mesh/elements/Loop.h"
#include "kernel/geometry/mesh/elements/Vertex.h"

#include <cassert>
#include <cstddef>
#include <vector>

namespace locus::kernel::geometry {

    /**
     * @brief Owns the canonical element storage used by an editable LEM mesh.
     *
     * LEMStorage stores vertices, edges, loops, and faces in stable index-based
     * arrays addressed through typed handles. It does not maintain derived
     * topology caches, adjacency tables, triangulation data, or acceleration
     * structures.
     *
     * The storage is intentionally low-level. Higher-level topology rules,
     * connectivity editing, radial cycle management, and face cycle construction
     * are responsibilities of LEM and LEMEditor.
     */
    class LEMStorage {
    public:
        /**
         * @brief Adds a vertex element to the storage.
         *
         * @param vertex Vertex data to append.
         * @return Handle referencing the appended vertex.
         */
        VertexHandle add_vertex(const Vertex& vertex)
        {
            const auto index = static_cast<std::size_t>(vertices_.size());
            vertices_.push_back(vertex);
            return VertexHandle(index);
        }

        /**
         * @brief Adds an edge element to the storage.
         *
         * @param edge Edge data to append.
         * @return Handle referencing the appended edge.
         */
        EdgeHandle add_edge(const Edge& edge)
        {
            const auto index = static_cast<std::size_t>(edges_.size());
            edges_.push_back(edge);
            return EdgeHandle(index);
        }

        /**
         * @brief Adds a loop element to the storage.
         *
         * @param loop Loop data to append.
         * @return Handle referencing the appended loop.
         */
        LoopHandle add_loop(const Loop& loop)
        {
            const auto index = static_cast<std::size_t>(loops_.size());
            loops_.push_back(loop);
            return LoopHandle(index);
        }

        /**
         * @brief Adds a face element to the storage.
         *
         * @param face Face data to append.
         * @return Handle referencing the appended face.
         */
        FaceHandle add_face(const Face& face)
        {
            const auto index = static_cast<std::size_t>(faces_.size());
            faces_.push_back(face);
            return FaceHandle(index);
        }

        /**
         * @brief Returns mutable access to a vertex.
         *
         * @param handle Vertex handle.
         * @return Mutable vertex reference.
         */
        [[nodiscard]] Vertex& vertex(VertexHandle handle)
        {
            assert(is_valid(handle));
            return vertices_[handle.id.value];
        }

        /**
         * @brief Returns read-only access to a vertex.
         *
         * @param handle Vertex handle.
         * @return Read-only vertex reference.
         */
        [[nodiscard]] const Vertex& vertex(VertexHandle handle) const
        {
            assert(is_valid(handle));
            return vertices_[handle.id.value];
        }

        /**
         * @brief Returns mutable access to an edge.
         *
         * @param handle Edge handle.
         * @return Mutable edge reference.
         */
        [[nodiscard]] Edge& edge(EdgeHandle handle)
        {
            assert(is_valid(handle));
            return edges_[handle.id.value];
        }

        /**
         * @brief Returns read-only access to an edge.
         *
         * @param handle Edge handle.
         * @return Read-only edge reference.
         */
        [[nodiscard]] const Edge& edge(EdgeHandle handle) const
        {
            assert(is_valid(handle));
            return edges_[handle.id.value];
        }

        /**
         * @brief Returns mutable access to a loop.
         *
         * @param handle Loop handle.
         * @return Mutable loop reference.
         */
        [[nodiscard]] Loop& loop(LoopHandle handle)
        {
            assert(is_valid(handle));
            return loops_[handle.id.value];
        }

        /**
         * @brief Returns read-only access to a loop.
         *
         * @param handle Loop handle.
         * @return Read-only loop reference.
         */
        [[nodiscard]] const Loop& loop(LoopHandle handle) const
        {
            assert(is_valid(handle));
            return loops_[handle.id.value];
        }

        /**
         * @brief Returns mutable access to a face.
         *
         * @param handle Face handle.
         * @return Mutable face reference.
         */
        [[nodiscard]] Face& face(FaceHandle handle)
        {
            assert(is_valid(handle));
            return faces_[handle.id.value];
        }

        /**
         * @brief Returns read-only access to a face.
         *
         * @param handle Face handle.
         * @return Read-only face reference.
         */
        [[nodiscard]] const Face& face(FaceHandle handle) const
        {
            assert(is_valid(handle));
            return faces_[handle.id.value];
        }

        /**
         * @brief Checks whether a vertex handle references an active vertex slot.
         *
         * @param handle Vertex handle.
         * @return True when the handle references an active vertex.
         */
        [[nodiscard]] bool is_valid(VertexHandle handle) const
        {
            return handle.is_valid()
                && handle.id.value < vertices_.size()
                && !vertices_[handle.id.value].deleted;
        }

        /**
         * @brief Checks whether an edge handle references an active edge slot.
         *
         * @param handle Edge handle.
         * @return True when the handle references an active edge.
         */
        [[nodiscard]] bool is_valid(EdgeHandle handle) const
        {
            return handle.is_valid()
                && handle.id.value < edges_.size()
                && !edges_[handle.id.value].deleted;
        }

        /**
         * @brief Checks whether a loop handle references an active loop slot.
         *
         * @param handle Loop handle.
         * @return True when the handle references an active loop.
         */
        [[nodiscard]] bool is_valid(LoopHandle handle) const
        {
            return handle.is_valid()
                && handle.id.value < loops_.size()
                && !loops_[handle.id.value].deleted;
        }

        /**
         * @brief Checks whether a face handle references an active face slot.
         *
         * @param handle Face handle.
         * @return True when the handle references an active face.
         */
        [[nodiscard]] bool is_valid(FaceHandle handle) const
        {
            return handle.is_valid()
                && handle.id.value < faces_.size()
                && !faces_[handle.id.value].deleted;
        }

        /**
         * @brief Returns the number of stored vertex slots.
         *
         * @return Number of stored vertices, including deleted slots.
         */
        [[nodiscard]] std::size_t vertex_count() const
        {
            return vertices_.size();
        }

        /**
         * @brief Returns the number of stored edge slots.
         *
         * @return Number of stored edges, including deleted slots.
         */
        [[nodiscard]] std::size_t edge_count() const
        {
            return edges_.size();
        }

        /**
         * @brief Returns the number of stored loop slots.
         *
         * @return Number of stored loops, including deleted slots.
         */
        [[nodiscard]] std::size_t loop_count() const
        {
            return loops_.size();
        }

        /**
         * @brief Returns the number of stored face slots.
         *
         * @return Number of stored faces, including deleted slots.
         */
        [[nodiscard]] std::size_t face_count() const
        {
            return faces_.size();
        }

        /**
         * @brief Checks whether the storage contains no element slots.
         *
         * @return True when all element arrays are empty.
         */
        [[nodiscard]] bool empty() const
        {
            return vertices_.empty()
                && edges_.empty()
                && loops_.empty()
                && faces_.empty();
        }

        /**
         * @brief Removes all stored elements.
         */
        void clear()
        {
            vertices_.clear();
            edges_.clear();
            loops_.clear();
            faces_.clear();
        }

        /**
         * @brief Returns all stored vertices.
         *
         * @return Read-only vertex array.
         */
        [[nodiscard]] const std::vector<Vertex>& vertices() const
        {
            return vertices_;
        }

        /**
         * @brief Returns all stored edges.
         *
         * @return Read-only edge array.
         */
        [[nodiscard]] const std::vector<Edge>& edges() const
        {
            return edges_;
        }

        /**
         * @brief Returns all stored loops.
         *
         * @return Read-only loop array.
         */
        [[nodiscard]] const std::vector<Loop>& loops() const
        {
            return loops_;
        }

        /**
         * @brief Returns all stored faces.
         *
         * @return Read-only face array.
         */
        [[nodiscard]] const std::vector<Face>& faces() const
        {
            return faces_;
        }

        /**
         * @brief Returns mutable access to all stored vertices.
         *
         * @return Mutable vertex array.
         */
        [[nodiscard]] std::vector<Vertex>& vertices()
        {
            return vertices_;
        }

        /**
         * @brief Returns mutable access to all stored edges.
         *
         * @return Mutable edge array.
         */
        [[nodiscard]] std::vector<Edge>& edges()
        {
            return edges_;
        }

        /**
         * @brief Returns mutable access to all stored loops.
         *
         * @return Mutable loop array.
         */
        [[nodiscard]] std::vector<Loop>& loops()
        {
            return loops_;
        }

        /**
         * @brief Returns mutable access to all stored faces.
         *
         * @return Mutable face array.
         */
        [[nodiscard]] std::vector<Face>& faces()
        {
            return faces_;
        }

    private:
        std::vector<Vertex> vertices_;
        std::vector<Edge> edges_;
        std::vector<Loop> loops_;
        std::vector<Face> faces_;
    };

}