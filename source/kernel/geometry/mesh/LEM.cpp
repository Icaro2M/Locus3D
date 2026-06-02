/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/geometry/mesh/LEM.h"

#include <glm/geometric.hpp>

#include <cassert>
#include <cstddef>
#include <vector>

namespace locus::kernel::geometry
{
    namespace
    {
        [[nodiscard]] bool matchesEdge(const Edge& edge, VertexHandle vertexA, VertexHandle vertexB)
        {
            return (edge.vertexA == vertexA && edge.vertexB == vertexB)
                || (edge.vertexA == vertexB && edge.vertexB == vertexA);
        }

        [[nodiscard]] bool containsVertex(
            const std::vector<VertexHandle>& vertices,
            VertexHandle target,
            std::size_t end)
        {
            for (std::size_t i = 0; i < end; ++i)
            {
                if (vertices[i] == target)
                {
                    return true;
                }
            }

            return false;
        }

        [[nodiscard]] bool containsEdgePair(
            const std::vector<VertexHandle>& vertices,
            VertexHandle vertexA,
            VertexHandle vertexB,
            std::size_t end)
        {
            for (std::size_t i = 0; i < end; ++i)
            {
                const VertexHandle currentA = vertices[i];
                const VertexHandle currentB = vertices[(i + 1) % vertices.size()];

                if ((currentA == vertexA && currentB == vertexB)
                    || (currentA == vertexB && currentB == vertexA))
                {
                    return true;
                }
            }

            return false;
        }

        /*
         * Rejects boundaries that would create degenerate local topology.
         * This validation is intentionally conservative for the initial LEM version.
         */
        [[nodiscard]] bool hasValidFaceBoundary(
            const LEM& mesh,
            const std::vector<VertexHandle>& vertices)
        {
            if (vertices.size() < 3)
            {
                return false;
            }

            for (std::size_t i = 0; i < vertices.size(); ++i)
            {
                const VertexHandle current = vertices[i];
                const VertexHandle next = vertices[(i + 1) % vertices.size()];

                if (!mesh.isValid(current) || current == next)
                {
                    return false;
                }

                if (containsVertex(vertices, current, i))
                {
                    return false;
                }

                if (containsEdgePair(vertices, current, next, i))
                {
                    return false;
                }
            }

            return true;
        }

        /*
         * Computes a polygon normal using Newell's method.
         * The returned normal is only a geometric helper for the current face state.
         */
        [[nodiscard]] glm::vec3 computeFaceNormal(
            const std::vector<Vertex>& meshVertices,
            const std::vector<VertexHandle>& faceVertices)
        {
            glm::vec3 normal{ 0.0f, 0.0f, 0.0f };

            for (std::size_t i = 0; i < faceVertices.size(); ++i)
            {
                const auto currentIndex = faceVertices[i].id.value;
                const auto nextIndex = faceVertices[(i + 1) % faceVertices.size()].id.value;

                const glm::vec3& current = meshVertices[currentIndex].position;
                const glm::vec3& next = meshVertices[nextIndex].position;

                normal.x += (current.y - next.y) * (current.z + next.z);
                normal.y += (current.z - next.z) * (current.x + next.x);
                normal.z += (current.x - next.x) * (current.y + next.y);
            }

            const float length = glm::length(normal);
            if (length <= 0.0f)
            {
                return glm::vec3{ 0.0f, 1.0f, 0.0f };
            }

            return normal / length;
        }
    }

    VertexHandle LEM::addVertex(const glm::vec3& position)
    {
        const auto index = static_cast<IdValue>(vertices_.size());

        Vertex vertex{};
        vertex.position = position;

        vertices_.push_back(vertex);

        return VertexHandle(index);
    }

    EdgeHandle LEM::findEdge(VertexHandle vertexA, VertexHandle vertexB) const
    {
        if (!isValid(vertexA) || !isValid(vertexB))
        {
            return EdgeHandle{};
        }

        for (std::size_t index = 0; index < edges_.size(); ++index)
        {
            const Edge& edge = edges_[index];

            if (!edge.deleted && matchesEdge(edge, vertexA, vertexB))
            {
                return EdgeHandle(static_cast<IdValue>(index));
            }
        }

        return EdgeHandle{};
    }

    /*
     * Edges are non-directional in LEM. Reuse an existing edge when the same
     * vertex pair is already connected.
     */
    EdgeHandle LEM::findOrCreateEdge(VertexHandle vertexA, VertexHandle vertexB)
    {
        if (!isValid(vertexA) || !isValid(vertexB) || vertexA == vertexB)
        {
            return EdgeHandle{};
        }

        const EdgeHandle existingEdge = findEdge(vertexA, vertexB);
        if (existingEdge.isValid())
        {
            return existingEdge;
        }

        const auto index = static_cast<IdValue>(edges_.size());
        const EdgeHandle edgeHandle(index);

        Edge edge{};
        edge.vertexA = vertexA;
        edge.vertexB = vertexB;

        edges_.push_back(edge);

        if (vertices_[vertexA.id.value].edge.isInvalid())
        {
            vertices_[vertexA.id.value].edge = edgeHandle;
        }

        if (vertices_[vertexB.id.value].edge.isInvalid())
        {
            vertices_[vertexB.id.value].edge = edgeHandle;
        }

        return edgeHandle;
    }

    /*
     * Creates one loop per boundary vertex, links the face cycle, and inserts
     * each loop into the radial cycle of its edge.
     */
    FaceHandle LEM::addFace(const std::vector<VertexHandle>& vertices)
    {
        if (!hasValidFaceBoundary(*this, vertices))
        {
            return FaceHandle{};
        }

        std::vector<EdgeHandle> faceEdges;
        faceEdges.reserve(vertices.size());

        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            const VertexHandle vertexA = vertices[i];
            const VertexHandle vertexB = vertices[(i + 1) % vertices.size()];

            const EdgeHandle edgeHandle = findOrCreateEdge(vertexA, vertexB);
            if (edgeHandle.isInvalid())
            {
                return FaceHandle{};
            }

            faceEdges.push_back(edgeHandle);
        }

        const auto faceIndex = static_cast<IdValue>(faces_.size());
        const FaceHandle faceHandle(faceIndex);

        Face face{};
        face.normal = computeFaceNormal(vertices_, vertices);

        faces_.push_back(face);

        std::vector<LoopHandle> faceLoopHandles;
        faceLoopHandles.reserve(vertices.size());

        for (std::size_t i = 0; i < vertices.size(); ++i)
        {
            const auto loopIndex = static_cast<IdValue>(loops_.size());
            const LoopHandle loopHandle(loopIndex);

            Loop loop{};
            loop.vertex = vertices[i];
            loop.edge = faceEdges[i];
            loop.face = faceHandle;

            loops_.push_back(loop);
            faceLoopHandles.push_back(loopHandle);

            Edge& edge = edges_[faceEdges[i].id.value];

            if (edge.loop.isInvalid())
            {
                edge.loop = loopHandle;
                loops_[loopIndex].radialNext = loopHandle;
                loops_[loopIndex].radialPrevious = loopHandle;
                continue;
            }

            /*
             * Insert the new loop before the radial entry loop, preserving a
             * circular doubly-linked radial cycle around the edge.
             */
            const LoopHandle entry = edge.loop;
            assert(isValid(entry));

            const LoopHandle previous = loops_[entry.id.value].radialPrevious;
            assert(isValid(previous));

            loops_[loopIndex].radialNext = entry;
            loops_[loopIndex].radialPrevious = previous;

            loops_[previous.id.value].radialNext = loopHandle;
            loops_[entry.id.value].radialPrevious = loopHandle;
        }

        for (std::size_t i = 0; i < faceLoopHandles.size(); ++i)
        {
            const LoopHandle current = faceLoopHandles[i];
            const LoopHandle next = faceLoopHandles[(i + 1) % faceLoopHandles.size()];
            const LoopHandle previous = faceLoopHandles[(i + faceLoopHandles.size() - 1) % faceLoopHandles.size()];

            loops_[current.id.value].next = next;
            loops_[current.id.value].previous = previous;
        }

        faces_[faceIndex].loop = faceLoopHandles.front();

        return faceHandle;
    }

    Vertex& LEM::vertex(VertexHandle handle)
    {
        assert(isValid(handle));
        return vertices_[handle.id.value];
    }

    const Vertex& LEM::vertex(VertexHandle handle) const
    {
        assert(isValid(handle));
        return vertices_[handle.id.value];
    }

    Edge& LEM::edge(EdgeHandle handle)
    {
        assert(isValid(handle));
        return edges_[handle.id.value];
    }

    const Edge& LEM::edge(EdgeHandle handle) const
    {
        assert(isValid(handle));
        return edges_[handle.id.value];
    }

    Loop& LEM::loop(LoopHandle handle)
    {
        assert(isValid(handle));
        return loops_[handle.id.value];
    }

    const Loop& LEM::loop(LoopHandle handle) const
    {
        assert(isValid(handle));
        return loops_[handle.id.value];
    }

    Face& LEM::face(FaceHandle handle)
    {
        assert(isValid(handle));
        return faces_[handle.id.value];
    }

    const Face& LEM::face(FaceHandle handle) const
    {
        assert(isValid(handle));
        return faces_[handle.id.value];
    }

    bool LEM::isValid(VertexHandle handle) const
    {
        return handle.isValid()
            && handle.id.value < vertices_.size()
            && !vertices_[handle.id.value].deleted;
    }

    bool LEM::isValid(EdgeHandle handle) const
    {
        return handle.isValid()
            && handle.id.value < edges_.size()
            && !edges_[handle.id.value].deleted;
    }

    bool LEM::isValid(LoopHandle handle) const
    {
        return handle.isValid()
            && handle.id.value < loops_.size()
            && !loops_[handle.id.value].deleted;
    }

    bool LEM::isValid(FaceHandle handle) const
    {
        return handle.isValid()
            && handle.id.value < faces_.size()
            && !faces_[handle.id.value].deleted;
    }

    std::vector<LoopHandle> LEM::faceLoops(FaceHandle handle) const
    {
        assert(isValid(handle));

        std::vector<LoopHandle> result;

        const LoopHandle firstLoop = faces_[handle.id.value].loop;
        if (firstLoop.isInvalid())
        {
            return result;
        }

        LoopHandle current = firstLoop;

        do
        {
            assert(isValid(current));

            if (!isValid(current))
            {
                return result;
            }

            result.push_back(current);
            current = loops_[current.id.value].next;
        } while (current != firstLoop && result.size() < loops_.size());

        return result;
    }

    std::size_t LEM::vertexCount() const
    {
        return vertices_.size();
    }

    std::size_t LEM::edgeCount() const
    {
        return edges_.size();
    }

    std::size_t LEM::loopCount() const
    {
        return loops_.size();
    }

    std::size_t LEM::faceCount() const
    {
        return faces_.size();
    }

    bool LEM::empty() const
    {
        return vertices_.empty()
            && edges_.empty()
            && loops_.empty()
            && faces_.empty();
    }

    void LEM::clear()
    {
        vertices_.clear();
        edges_.clear();
        loops_.clear();
        faces_.clear();
    }

    const std::vector<Vertex>& LEM::vertices() const
    {
        return vertices_;
    }

    const std::vector<Edge>& LEM::edges() const
    {
        return edges_;
    }

    const std::vector<Loop>& LEM::loops() const
    {
        return loops_;
    }

    const std::vector<Face>& LEM::faces() const
    {
        return faces_;
    }
}