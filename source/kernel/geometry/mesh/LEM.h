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

#include <glm/glm.hpp>

#include <cstddef>
#include <vector>

namespace locus::kernel::geometry
{
    /**
     * @brief Editable polygon mesh representation used by the geometry kernel.
     *
     * LEM stores editable topology through vertices, edges, loops, and faces.
     * It represents polygonal faces directly; derived triangulation is expected
     * to be built by rendering, export, or analysis systems outside this class.
     */
    class LEM
    {
    public:
        /**
         * @brief Adds a loose vertex to the mesh.
         *
         * @param position Vertex position in object space.
         * @return Handle referencing the created vertex.
         */
        VertexHandle addVertex(const glm::vec3& position);

        /**
         * @brief Finds an existing non-directional edge between two vertices.
         *
         * @param vertexA First endpoint vertex.
         * @param vertexB Second endpoint vertex.
         * @return Handle referencing the edge, or an invalid handle when none exists.
         */
        [[nodiscard]] EdgeHandle findEdge(VertexHandle vertexA, VertexHandle vertexB) const;

        /**
         * @brief Finds or creates a non-directional edge between two vertices.
         *
         * @param vertexA First endpoint vertex.
         * @param vertexB Second endpoint vertex.
         * @return Handle referencing the existing or newly created edge.
         */
        EdgeHandle findOrCreateEdge(VertexHandle vertexA, VertexHandle vertexB);

        /**
         * @brief Adds a polygonal face bounded by the given vertices.
         *
         * The method creates one loop per face corner, links the face cycle, and
         * inserts each loop into the radial cycle of its edge.
         *
         * @param vertices Ordered face vertices.
         * @return Handle referencing the created face.
         * @note The vertex order defines the face winding and normal direction.
         */
        FaceHandle addFace(const std::vector<VertexHandle>& vertices);

        /**
         * @brief Returns mutable access to a vertex.
         *
         * @param handle Vertex handle.
         * @return Mutable vertex reference.
         */
        [[nodiscard]] Vertex& vertex(VertexHandle handle);

        /**
         * @brief Returns read-only access to a vertex.
         *
         * @param handle Vertex handle.
         * @return Read-only vertex reference.
         */
        [[nodiscard]] const Vertex& vertex(VertexHandle handle) const;

        /**
         * @brief Returns mutable access to an edge.
         *
         * @param handle Edge handle.
         * @return Mutable edge reference.
         */
        [[nodiscard]] Edge& edge(EdgeHandle handle);

        /**
         * @brief Returns read-only access to an edge.
         *
         * @param handle Edge handle.
         * @return Read-only edge reference.
         */
        [[nodiscard]] const Edge& edge(EdgeHandle handle) const;

        /**
         * @brief Returns mutable access to a loop.
         *
         * @param handle Loop handle.
         * @return Mutable loop reference.
         */
        [[nodiscard]] Loop& loop(LoopHandle handle);

        /**
         * @brief Returns read-only access to a loop.
         *
         * @param handle Loop handle.
         * @return Read-only loop reference.
         */
        [[nodiscard]] const Loop& loop(LoopHandle handle) const;

        /**
         * @brief Returns mutable access to a face.
         *
         * @param handle Face handle.
         * @return Mutable face reference.
         */
        [[nodiscard]] Face& face(FaceHandle handle);

        /**
         * @brief Returns read-only access to a face.
         *
         * @param handle Face handle.
         * @return Read-only face reference.
         */
        [[nodiscard]] const Face& face(FaceHandle handle) const;

        /**
         * @brief Checks whether a vertex handle references an active vertex.
         *
         * @param handle Vertex handle.
         * @return True when the handle references an active vertex.
         */
        [[nodiscard]] bool isValid(VertexHandle handle) const;

        /**
         * @brief Checks whether an edge handle references an active edge.
         *
         * @param handle Edge handle.
         * @return True when the handle references an active edge.
         */
        [[nodiscard]] bool isValid(EdgeHandle handle) const;

        /**
         * @brief Checks whether a loop handle references an active loop.
         *
         * @param handle Loop handle.
         * @return True when the handle references an active loop.
         */
        [[nodiscard]] bool isValid(LoopHandle handle) const;

        /**
         * @brief Checks whether a face handle references an active face.
         *
         * @param handle Face handle.
         * @return True when the handle references an active face.
         */
        [[nodiscard]] bool isValid(FaceHandle handle) const;

        /**
         * @brief Returns the loops that form a face boundary.
         *
         * @param handle Face handle.
         * @return Ordered loop handles around the face.
         */
        [[nodiscard]] std::vector<LoopHandle> faceLoops(FaceHandle handle) const;

        /**
         * @brief Returns the number of vertex slots.
         *
         * @return Number of stored vertices.
         */
        [[nodiscard]] std::size_t vertexCount() const;

        /**
         * @brief Returns the number of edge slots.
         *
         * @return Number of stored edges.
         */
        [[nodiscard]] std::size_t edgeCount() const;

        /**
         * @brief Returns the number of loop slots.
         *
         * @return Number of stored loops.
         */
        [[nodiscard]] std::size_t loopCount() const;

        /**
         * @brief Returns the number of face slots.
         *
         * @return Number of stored faces.
         */
        [[nodiscard]] std::size_t faceCount() const;

        /**
         * @brief Checks whether the mesh contains no elements.
         *
         * @return True when all element arrays are empty.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Removes all mesh elements.
         */
        void clear();

        /**
         * @brief Returns all stored vertices.
         *
         * @return Read-only vertex array.
         */
        [[nodiscard]] const std::vector<Vertex>& vertices() const;

        /**
         * @brief Returns all stored edges.
         *
         * @return Read-only edge array.
         */
        [[nodiscard]] const std::vector<Edge>& edges() const;

        /**
         * @brief Returns all stored loops.
         *
         * @return Read-only loop array.
         */
        [[nodiscard]] const std::vector<Loop>& loops() const;

        /**
         * @brief Returns all stored faces.
         *
         * @return Read-only face array.
         */
        [[nodiscard]] const std::vector<Face>& faces() const;

    private:
        std::vector<Vertex> vertices_;
        std::vector<Edge> edges_;
        std::vector<Loop> loops_;
        std::vector<Face> faces_;
    };
}
