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
     * @brief Owns the canonical element arrays used by a LEM mesh.
     *
     * LEMStorage is intentionally limited to low-level storage concerns. It stores
     * vertices, edges, loops, and faces in index-addressed arrays and provides
     * typed-handle access to those arrays.
     *
     * Topology rules, radial cycle maintenance, face cycle construction, editing
     * operations, derived caches, triangulation, and acceleration structures are
     * intentionally kept outside this class.
     */
    class LEMStorage {
    public:
        /**
         * @brief Adds a vertex element.
         *
         * @param vertex Vertex data to append.
         * @return Handle referencing the appended vertex.
         */
        VertexHandle add_vertex(const Vertex& vertex)
        {
            const auto index = static_cast<IdValue>(vertices_.size());
            vertices_.push_back(vertex);
            return VertexHandle(index);
        }

        /**
         * @brief Adds an edge element.
         *
         * @param edge Edge data to append.
         * @return Handle referencing the appended edge.
         */
        EdgeHandle add_edge(const Edge& edge)
        {
            const auto index = static_cast<IdValue>(edges_.size());
            edges_.push_back(edge);
            return EdgeHandle(index);
        }

        /**
         * @brief Adds a loop element.
         *
         * @param loop Loop data to append.
         * @return Handle referencing the appended loop.
         */
        LoopHandle add_loop(const Loop& loop)
        {
            const auto index = static_cast<IdValue>(loops_.size());
            loops_.push_back(loop);
            return LoopHandle(index);
        }

        /**
         * @brief Adds a face element.
         *
         * @param face Face data to append.
         * @return Handle referencing the appended face.
         */
        FaceHandle add_face(const Face& face)
        {
            const auto index = static_cast<IdValue>(faces_.size());
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
         * @brief Checks whether a vertex handle references an active vertex.
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
         * @brief Checks whether an edge handle references an active edge.
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
         * @brief Checks whether a loop handle references an active loop.
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
         * @brief Checks whether a face handle references an active face.
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
         * @return Number of stored vertices.
         */
        [[nodiscard]] std::size_t vertex_count() const
        {
            return vertices_.size();
        }

        /**
         * @brief Returns the number of stored edge slots.
         *
         * @return Number of stored edges.
         */
        [[nodiscard]] std::size_t edge_count() const
        {
            return edges_.size();
        }

        /**
         * @brief Returns the number of stored loop slots.
         *
         * @return Number of stored loops.
         */
        [[nodiscard]] std::size_t loop_count() const
        {
            return loops_.size();
        }

        /**
         * @brief Returns the number of stored face slots.
         *
         * @return Number of stored faces.
         */
        [[nodiscard]] std::size_t face_count() const
        {
            return faces_.size();
        }

        /**
         * @brief Checks whether all element arrays are empty.
         *
         * @return True when no element slots are stored.
         */
        [[nodiscard]] bool empty() const
        {
            return vertices_.empty()
                && edges_.empty()
                && loops_.empty()
                && faces_.empty();
        }

        /**
         * @brief Removes all stored element slots.
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

    private:
        std::vector<Vertex> vertices_;
        std::vector<Edge> edges_;
        std::vector<Loop> loops_;
        std::vector<Face> faces_;
    };

}