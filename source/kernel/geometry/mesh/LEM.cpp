/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/LEM.h"

#include <glm/geometric.hpp>

#include <cassert>
#include <cstddef>
#include <vector>

namespace locus::kernel::geometry {
    namespace {

        [[nodiscard]] bool matches_edge(const Edge& edge, VertexHandle vertexA, VertexHandle vertexB)
        {
            return (edge.vertexA == vertexA && edge.vertexB == vertexB)
                || (edge.vertexA == vertexB && edge.vertexB == vertexA);
        }

        [[nodiscard]] bool contains_vertex(
            const std::vector<VertexHandle>& vertices,
            VertexHandle target,
            std::size_t end)
        {
            for (std::size_t i = 0; i < end; ++i) {
                if (vertices[i] == target) {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool contains_edge_pair(
            const std::vector<VertexHandle>& vertices,
            VertexHandle vertexA,
            VertexHandle vertexB,
            std::size_t end)
        {
            for (std::size_t i = 0; i < end; ++i) {
                const VertexHandle currentA = vertices[i];
                const VertexHandle currentB = vertices[(i + 1) % vertices.size()];

                if ((currentA == vertexA && currentB == vertexB)
                    || (currentA == vertexB && currentB == vertexA)) {
                    return true;
                }
            }

            return false;
        }

        /**
         * @brief Checks whether a candidate polygon boundary can form a valid face.
         *
         * The validation is intentionally conservative for the current LEM version.
         *
         * @param mesh Mesh used to validate vertex handles.
         * @param vertices Ordered candidate face vertices.
         * @return True when the boundary is accepted.
         */
        [[nodiscard]] bool has_valid_face_boundary(
            const LEM& mesh,
            const std::vector<VertexHandle>& vertices)
        {
            if (vertices.size() < 3) {
                return false;
            }

            for (std::size_t i = 0; i < vertices.size(); ++i) {
                const VertexHandle current = vertices[i];
                const VertexHandle next = vertices[(i + 1) % vertices.size()];

                if (!mesh.is_valid(current) || current == next) {
                    return false;
                }

                if (contains_vertex(vertices, current, i)) {
                    return false;
                }

                if (contains_edge_pair(vertices, current, next, i)) {
                    return false;
                }
            }

            return true;
        }

        /**
         * @brief Computes a polygon normal using Newell's method.
         *
         * @param meshVertices Stored mesh vertices.
         * @param faceVertices Ordered face vertex handles.
         * @return Unit face normal, or a safe fallback normal for degenerate input.
         */
        [[nodiscard]] glm::vec3 compute_face_normal(
            const std::vector<Vertex>& meshVertices,
            const std::vector<VertexHandle>& faceVertices)
        {
            glm::vec3 normal{ 0.0f, 0.0f, 0.0f };

            for (std::size_t i = 0; i < faceVertices.size(); ++i) {
                const auto currentIndex = faceVertices[i].id.value;
                const auto nextIndex = faceVertices[(i + 1) % faceVertices.size()].id.value;

                const glm::vec3& current = meshVertices[currentIndex].position;
                const glm::vec3& next = meshVertices[nextIndex].position;

                normal.x += (current.y - next.y) * (current.z + next.z);
                normal.y += (current.z - next.z) * (current.x + next.x);
                normal.z += (current.x - next.x) * (current.y + next.y);
            }

            const float length = glm::length(normal);

            if (length <= 0.0f) {
                return glm::vec3{ 0.0f, 1.0f, 0.0f };
            }

            return normal / length;
        }

    }

    VertexHandle LEM::add_vertex(const glm::vec3& position)
    {
        Vertex vertex{};
        vertex.position = position;

        return storage_.add_vertex(vertex);
    }

    EdgeHandle LEM::find_edge(VertexHandle vertexA, VertexHandle vertexB) const
    {
        if (!is_valid(vertexA) || !is_valid(vertexB)) {
            return EdgeHandle{};
        }

        const std::vector<Edge>& storedEdges = storage_.edges();

        for (std::size_t index = 0; index < storedEdges.size(); ++index) {
            const Edge& edge = storedEdges[index];

            if (!edge.deleted && matches_edge(edge, vertexA, vertexB)) {
                return EdgeHandle(static_cast<IdValue>(index));
            }
        }

        return EdgeHandle{};
    }

    EdgeHandle LEM::find_or_create_edge(VertexHandle vertexA, VertexHandle vertexB)
    {
        if (!is_valid(vertexA) || !is_valid(vertexB) || vertexA == vertexB) {
            return EdgeHandle{};
        }

        const EdgeHandle existingEdge = find_edge(vertexA, vertexB);

        if (existingEdge.is_valid()) {
            return existingEdge;
        }

        Edge edge{};
        edge.vertexA = vertexA;
        edge.vertexB = vertexB;

        const EdgeHandle edgeHandle = storage_.add_edge(edge);

        if (storage_.vertex(vertexA).edge.is_invalid()) {
            storage_.vertex(vertexA).edge = edgeHandle;
        }

        if (storage_.vertex(vertexB).edge.is_invalid()) {
            storage_.vertex(vertexB).edge = edgeHandle;
        }

        return edgeHandle;
    }

    FaceHandle LEM::add_face(const std::vector<VertexHandle>& vertices)
    {
        if (!has_valid_face_boundary(*this, vertices)) {
            return FaceHandle{};
        }

        std::vector<EdgeHandle> faceEdges;
        faceEdges.reserve(vertices.size());

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const VertexHandle vertexA = vertices[i];
            const VertexHandle vertexB = vertices[(i + 1) % vertices.size()];

            const EdgeHandle edgeHandle = find_or_create_edge(vertexA, vertexB);

            if (edgeHandle.is_invalid()) {
                return FaceHandle{};
            }

            faceEdges.push_back(edgeHandle);
        }

        Face face{};
        face.normal = compute_face_normal(storage_.vertices(), vertices);

        const FaceHandle faceHandle = storage_.add_face(face);

        std::vector<LoopHandle> faceLoopHandles;
        faceLoopHandles.reserve(vertices.size());

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            Loop loop{};
            loop.vertex = vertices[i];
            loop.edge = faceEdges[i];
            loop.face = faceHandle;

            const LoopHandle loopHandle = storage_.add_loop(loop);
            faceLoopHandles.push_back(loopHandle);

            Edge& edge = storage_.edge(faceEdges[i]);
            Loop& storedLoop = storage_.loop(loopHandle);

            if (edge.loop.is_invalid()) {
                edge.loop = loopHandle;
                storedLoop.radialNext = loopHandle;
                storedLoop.radialPrevious = loopHandle;
                continue;
            }

            const LoopHandle entry = edge.loop;

            assert(is_valid(entry));

            const LoopHandle previous = storage_.loop(entry).radialPrevious;

            assert(is_valid(previous));

            storedLoop.radialNext = entry;
            storedLoop.radialPrevious = previous;
            storage_.loop(previous).radialNext = loopHandle;
            storage_.loop(entry).radialPrevious = loopHandle;
        }

        for (std::size_t i = 0; i < faceLoopHandles.size(); ++i) {
            const LoopHandle current = faceLoopHandles[i];
            const LoopHandle next = faceLoopHandles[(i + 1) % faceLoopHandles.size()];
            const LoopHandle previous = faceLoopHandles[(i + faceLoopHandles.size() - 1) % faceLoopHandles.size()];

            storage_.loop(current).next = next;
            storage_.loop(current).previous = previous;
        }

        storage_.face(faceHandle).loop = faceLoopHandles.front();

        return faceHandle;
    }

    Vertex& LEM::vertex(VertexHandle handle)
    {
        return storage_.vertex(handle);
    }

    const Vertex& LEM::vertex(VertexHandle handle) const
    {
        return storage_.vertex(handle);
    }

    Edge& LEM::edge(EdgeHandle handle)
    {
        return storage_.edge(handle);
    }

    const Edge& LEM::edge(EdgeHandle handle) const
    {
        return storage_.edge(handle);
    }

    Loop& LEM::loop(LoopHandle handle)
    {
        return storage_.loop(handle);
    }

    const Loop& LEM::loop(LoopHandle handle) const
    {
        return storage_.loop(handle);
    }

    Face& LEM::face(FaceHandle handle)
    {
        return storage_.face(handle);
    }

    const Face& LEM::face(FaceHandle handle) const
    {
        return storage_.face(handle);
    }

    bool LEM::is_valid(VertexHandle handle) const
    {
        return storage_.is_valid(handle);
    }

    bool LEM::is_valid(EdgeHandle handle) const
    {
        return storage_.is_valid(handle);
    }

    bool LEM::is_valid(LoopHandle handle) const
    {
        return storage_.is_valid(handle);
    }

    bool LEM::is_valid(FaceHandle handle) const
    {
        return storage_.is_valid(handle);
    }

    std::vector<LoopHandle> LEM::face_loops(FaceHandle handle) const
    {
        assert(is_valid(handle));

        std::vector<LoopHandle> result;

        const LoopHandle firstLoop = storage_.face(handle).loop;

        if (firstLoop.is_invalid()) {
            return result;
        }

        LoopHandle current = firstLoop;

        do {
            assert(is_valid(current));

            if (!is_valid(current)) {
                return result;
            }

            result.push_back(current);
            current = storage_.loop(current).next;
        } while (current != firstLoop && result.size() < storage_.loop_count());

        return result;
    }

    std::size_t LEM::vertex_count() const
    {
        return storage_.vertex_count();
    }

    std::size_t LEM::edge_count() const
    {
        return storage_.edge_count();
    }

    std::size_t LEM::loop_count() const
    {
        return storage_.loop_count();
    }

    std::size_t LEM::face_count() const
    {
        return storage_.face_count();
    }

    bool LEM::empty() const
    {
        return storage_.empty();
    }

    void LEM::clear()
    {
        storage_.clear();
    }

    const std::vector<Vertex>& LEM::vertices() const
    {
        return storage_.vertices();
    }

    const std::vector<Edge>& LEM::edges() const
    {
        return storage_.edges();
    }

    const std::vector<Loop>& LEM::loops() const
    {
        return storage_.loops();
    }

    const std::vector<Face>& LEM::faces() const
    {
        return storage_.faces();
    }

}