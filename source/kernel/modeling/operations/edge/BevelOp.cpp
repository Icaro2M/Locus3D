/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/edge/BevelOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

#include <glm/geometric.hpp>

namespace locus::kernel::modeling {

    namespace {

        constexpr float bevelEpsilon =
            0.000001f;

        struct FaceSide {
            geometry::VertexHandle from{};
            geometry::VertexHandle to{};
            geometry::EdgeHandle edge{};
        };

        struct EdgeFaceCut {
            geometry::FaceHandle face{};
            geometry::EdgeHandle edge{};
            glm::vec3 faceNormal{ 0.0f };
            geometry::VertexHandle edgeVertexA{};
            geometry::VertexHandle edgeVertexB{};
            geometry::VertexHandle cutAtA{};
            geometry::VertexHandle cutAtB{};
            geometry::VertexHandle cutAtAToward{};
            geometry::VertexHandle cutAtBToward{};
            geometry::VertexHandle startCut{};
            geometry::VertexHandle endCut{};
        };

        [[nodiscard]]
        glm::vec3 point_toward(
            const geometry::LEM& mesh,
            geometry::VertexHandle origin,
            geometry::VertexHandle target,
            float distance)
        {
            const glm::vec3 originPosition =
                mesh.vertex(origin).position;
            const glm::vec3 targetPosition =
                mesh.vertex(target).position;
            const glm::vec3 delta =
                targetPosition - originPosition;
            const float length =
                glm::length(delta);

            if (length <= bevelEpsilon) {
                return originPosition;
            }

            const float clampedDistance =
                std::min(
                    distance,
                    length * 0.45f);

            return originPosition +
                (delta / length) * clampedDistance;
        }

        [[nodiscard]]
        glm::vec3 normalized_or_zero(const glm::vec3& value)
        {
            const float length =
                glm::length(value);

            if (length <= bevelEpsilon) {
                return glm::vec3{ 0.0f };
            }

            return value / length;
        }

        [[nodiscard]]
        glm::vec3 polygon_normal(
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& vertices)
        {
            if (vertices.size() < 3) {
                return glm::vec3{ 0.0f };
            }

            glm::vec3 normal{ 0.0f };

            for (std::size_t index = 0; index < vertices.size(); ++index) {
                const geometry::VertexHandle currentHandle =
                    vertices[index];
                const geometry::VertexHandle nextHandle =
                    vertices[(index + 1) % vertices.size()];

                if (!mesh.is_valid(currentHandle) ||
                    !mesh.is_valid(nextHandle)) {
                    return glm::vec3{ 0.0f };
                }

                const glm::vec3& current =
                    mesh.vertex(currentHandle).position;
                const glm::vec3& next =
                    mesh.vertex(nextHandle).position;

                normal.x += (current.y - next.y) * (current.z + next.z);
                normal.y += (current.z - next.z) * (current.x + next.x);
                normal.z += (current.x - next.x) * (current.y + next.y);
            }

            return normalized_or_zero(normal);
        }

        [[nodiscard]]
        std::vector<geometry::VertexHandle> oriented_face_vertices(
            const geometry::LEM& mesh,
            std::vector<geometry::VertexHandle> vertices,
            const glm::vec3& referenceNormal)
        {
            const glm::vec3 faceNormal =
                polygon_normal(
                    mesh,
                    vertices);

            if (glm::dot(faceNormal, referenceNormal) < 0.0f) {
                std::reverse(
                    vertices.begin(),
                    vertices.end());
            }

            return vertices;
        }

        template<typename Handle>
        [[nodiscard]]
        bool contains_handle(
            const std::vector<Handle>& handles,
            Handle handle)
        {
            return std::find(
                handles.begin(),
                handles.end(),
                handle) != handles.end();
        }

        [[nodiscard]]
        bool has_duplicate_vertices(
            const std::vector<geometry::VertexHandle>& vertices)
        {
            for (std::size_t i = 0; i < vertices.size(); ++i) {
                if (!vertices[i].is_valid()) {
                    return true;
                }

                for (std::size_t j = i + 1; j < vertices.size(); ++j) {
                    if (vertices[i] == vertices[j]) {
                        return true;
                    }
                }
            }

            return false;
        }

        [[nodiscard]]
        bool has_effective_area(
            const geometry::LEM& mesh,
            const std::vector<geometry::VertexHandle>& vertices)
        {
            if (vertices.size() < 3 || has_duplicate_vertices(vertices)) {
                return false;
            }

            const glm::vec3 origin =
                mesh.vertex(vertices.front()).position;
            glm::vec3 accumulated{ 0.0f };

            for (std::size_t i = 1; i + 1 < vertices.size(); ++i) {
                accumulated +=
                    glm::cross(
                        mesh.vertex(vertices[i]).position - origin,
                        mesh.vertex(vertices[i + 1]).position - origin);
            }

            return glm::length(accumulated) > bevelEpsilon;
        }

        [[nodiscard]]
        std::vector<FaceSide> face_sides(
            const geometry::LEM& mesh,
            geometry::FaceHandle face)
        {
            const std::vector<geometry::LoopHandle> loops =
                geometry::TopologyTraversal::face_loops(
                    mesh,
                    face);

            std::vector<FaceSide> sides;
            sides.reserve(loops.size());

            for (const geometry::LoopHandle loopHandle : loops) {
                if (!mesh.is_valid(loopHandle)) {
                    return {};
                }

                const geometry::Loop& loop =
                    mesh.loop(loopHandle);

                if (!mesh.is_valid(loop.vertex) ||
                    !mesh.is_valid(loop.edge) ||
                    !mesh.is_valid(loop.next)) {
                    return {};
                }

                const geometry::Loop& nextLoop =
                    mesh.loop(loop.next);

                if (!mesh.is_valid(nextLoop.vertex)) {
                    return {};
                }

                sides.push_back(
                    FaceSide{
                        loop.vertex,
                        nextLoop.vertex,
                        loop.edge });
            }

            return sides;
        }

        [[nodiscard]]
        const EdgeFaceCut* find_edge_face_cut(
            const std::vector<EdgeFaceCut>& cuts,
            geometry::FaceHandle face,
            geometry::EdgeHandle edge)
        {
            for (const EdgeFaceCut& cut : cuts) {
                if (cut.face == face && cut.edge == edge) {
                    return &cut;
                }
            }

            return nullptr;
        }

        [[nodiscard]]
        geometry::VertexHandle find_cut_at_vertex_toward(
            const std::vector<EdgeFaceCut>& cuts,
            geometry::VertexHandle vertex,
            geometry::VertexHandle toward)
        {
            for (const EdgeFaceCut& cut : cuts) {
                if (cut.edgeVertexA == vertex &&
                    cut.cutAtAToward == toward &&
                    cut.cutAtA.is_valid()) {
                    return cut.cutAtA;
                }

                if (cut.edgeVertexB == vertex &&
                    cut.cutAtBToward == toward &&
                    cut.cutAtB.is_valid()) {
                    return cut.cutAtB;
                }
            }

            return {};
        }

        [[nodiscard]]
        geometry::VertexHandle add_cut_vertex(
            geometry::LEM& mesh,
            geometry::LEMEditor& editor,
            geometry::VertexHandle origin,
            geometry::VertexHandle target,
            float width)
        {
            if (!mesh.is_valid(origin) ||
                !mesh.is_valid(target) ||
                origin == target) {
                return {};
            }

            return editor.add_vertex(
                point_toward(
                    mesh,
                    origin,
                    target,
                    width));
        }

        void cleanup_loose_original_topology(
            geometry::LEM& mesh,
            geometry::LEMEditor& editor,
            const std::vector<geometry::EdgeHandle>& edges,
            const std::vector<geometry::VertexHandle>& vertices)
        {
            for (const geometry::EdgeHandle edge : edges) {
                if (mesh.is_valid(edge)) {
                    editor.remove_edge_if_loose(edge);
                }
            }

            for (const geometry::VertexHandle vertex : vertices) {
                if (mesh.is_valid(vertex)) {
                    editor.remove_vertex_if_loose(vertex);
                }
            }
        }

        void collect_original_face_topology(
            const geometry::LEM& mesh,
            const std::vector<geometry::FaceHandle>& faces,
            std::vector<geometry::EdgeHandle>& outEdges,
            std::vector<geometry::VertexHandle>& outVertices)
        {
            for (const geometry::FaceHandle face : faces) {
                if (!mesh.is_valid(face)) {
                    continue;
                }

                for (const geometry::EdgeHandle edge :
                    geometry::TopologyTraversal::face_edges(mesh, face)) {
                    if (mesh.is_valid(edge) &&
                        !contains_handle(outEdges, edge)) {
                        outEdges.push_back(edge);
                    }
                }

                for (const geometry::VertexHandle vertex :
                    geometry::TopologyTraversal::face_vertices(mesh, face)) {
                    if (mesh.is_valid(vertex) &&
                        !contains_handle(outVertices, vertex)) {
                        outVertices.push_back(vertex);
                    }
                }
            }
        }

    } // namespace

    BevelOp::BevelOp(geometry::EdgeHandle edge, float width)
        : target_(BevelTarget::Edges)
        , edges_({ edge })
        , width_(width)
    {
    }

    BevelOp::BevelOp(
        std::vector<geometry::EdgeHandle> edges,
        float width)
        : target_(BevelTarget::Edges)
        , edges_(std::move(edges))
        , width_(width)
    {
    }

    BevelOp::BevelOp(
        std::vector<geometry::VertexHandle> vertices,
        float width)
        : target_(BevelTarget::SelectedVertices)
        , vertices_(std::move(vertices))
        , width_(width)
    {
    }

    BevelOp BevelOp::selected_edges(float width)
    {
        BevelOp op;
        op.set_target(BevelTarget::SelectedEdges);
        op.set_width(width);
        return op;
    }

    BevelOp BevelOp::selected_vertices(float width)
    {
        BevelOp op;
        op.set_target(BevelTarget::SelectedVertices);
        op.set_width(width);
        return op;
    }

    std::string_view BevelOp::name() const
    {
        return "BevelOp";
    }

    void BevelOp::set_target(BevelTarget target)
    {
        target_ = target;
    }

    BevelTarget BevelOp::target() const
    {
        return target_;
    }

    void BevelOp::set_width(float width)
    {
        width_ = width;
    }

    float BevelOp::width() const
    {
        return width_;
    }

    void BevelOp::set_edges(std::vector<geometry::EdgeHandle> edges)
    {
        edges_ = std::move(edges);
    }

    const std::vector<geometry::EdgeHandle>& BevelOp::edges() const
    {
        return edges_;
    }

    void BevelOp::clear_edges()
    {
        edges_.clear();
    }

    void BevelOp::set_vertices(std::vector<geometry::VertexHandle> vertices)
    {
        vertices_ = std::move(vertices);
    }

    const std::vector<geometry::VertexHandle>& BevelOp::vertices() const
    {
        return vertices_;
    }

    void BevelOp::clear_vertices()
    {
        vertices_.clear();
    }

    OperationResult BevelOp::execute_impl(OperationContext& context)
    {
        if (width_ <= bevelEpsilon) {
            return OperationResult::no_change(
                "Bevel operation has zero width.");
        }

        geometry::LEM& mesh =
            context.editable_mesh();

        const std::vector<geometry::EdgeHandle> targetEdges =
            collect_edges(mesh);

        if (targetEdges.empty()) {
            return OperationResult::no_change(
                "Bevel operation found no valid target edges.");
        }

        for (const geometry::EdgeHandle edge : targetEdges) {
            if (!geometry::TopologyTraversal::is_manifold_edge(mesh, edge)) {
                return OperationResult::no_change(
                    "Bevel operation only supports manifold edges.");
            }
        }

        const std::vector<geometry::VertexHandle> targetVertices =
            collect_vertices(
                mesh,
                targetEdges);

        std::vector<geometry::FaceHandle> targetFaces;
        for (const geometry::EdgeHandle edge : targetEdges) {
            for (const geometry::FaceHandle face :
                geometry::TopologyTraversal::edge_faces(mesh, edge)) {
                if (mesh.is_valid(face) &&
                    !contains_handle(targetFaces, face)) {
                    targetFaces.push_back(face);
                }
            }
        }

        for (const geometry::VertexHandle vertex : targetVertices) {
            for (const geometry::FaceHandle face :
                geometry::TopologyTraversal::vertex_faces(mesh, vertex)) {
                if (mesh.is_valid(face) &&
                    !contains_handle(targetFaces, face)) {
                    targetFaces.push_back(face);
                }
            }
        }

        if (targetFaces.empty()) {
            return OperationResult::no_change(
                "Bevel operation found no affected faces.");
        }

        std::vector<geometry::EdgeHandle> originalEdges;
        std::vector<geometry::VertexHandle> originalVertices;

        collect_original_face_topology(
            mesh,
            targetFaces,
            originalEdges,
            originalVertices);

        geometry::LEMEditor editor(mesh);
        std::vector<EdgeFaceCut> edgeFaceCuts;
        std::size_t rebuiltFaceCount = 0;

        for (const geometry::FaceHandle face : targetFaces) {
            if (!bevel_face(
                mesh,
                editor,
                face,
                targetVertices)) {
                continue;
            }

            ++rebuiltFaceCount;

            const std::vector<FaceSide> sides =
                face_sides(mesh, face);

            if (sides.size() < 3) {
                continue;
            }

            const std::size_t sideCount =
                sides.size();

            for (std::size_t index = 0; index < sideCount; ++index) {
                const FaceSide& side =
                    sides[index];

                if (!contains_handle(targetEdges, side.edge)) {
                    continue;
                }

                const FaceSide& previousSide =
                    sides[(index + sideCount - 1) % sideCount];
                const FaceSide& nextSide =
                    sides[(index + 1) % sideCount];

                const geometry::VertexHandle startCut =
                    add_cut_vertex(
                        mesh,
                        editor,
                        side.from,
                        previousSide.from,
                        width_);

                const geometry::VertexHandle endCut =
                    add_cut_vertex(
                        mesh,
                        editor,
                        side.to,
                        nextSide.to,
                        width_);

                if (!mesh.is_valid(startCut) ||
                    !mesh.is_valid(endCut) ||
                    startCut == endCut) {
                    continue;
                }

                const geometry::Edge& edgeElement =
                    mesh.edge(side.edge);
                const bool forward =
                    side.from == edgeElement.vertexA &&
                    side.to == edgeElement.vertexB;

                edgeFaceCuts.push_back(
                    EdgeFaceCut{
                        face,
                        side.edge,
                        mesh.face(face).normal,
                        edgeElement.vertexA,
                        edgeElement.vertexB,
                        forward ? startCut : endCut,
                        forward ? endCut : startCut,
                        forward ? previousSide.from : nextSide.to,
                        forward ? nextSide.to : previousSide.from,
                        startCut,
                        endCut });
            }
        }

        if (rebuiltFaceCount == 0 || edgeFaceCuts.empty()) {
            return OperationResult::no_change(
                "Bevel operation did not find rebuildable edge regions.");
        }

        std::size_t createdFaceCount = 0;

        for (const geometry::FaceHandle face : targetFaces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            const std::vector<FaceSide> sides =
                face_sides(mesh, face);

            if (sides.size() < 3) {
                continue;
            }

            std::vector<geometry::VertexHandle> rebuiltVertices;
            rebuiltVertices.reserve(sides.size() * 2);

            for (std::size_t index = 0; index < sides.size(); ++index) {
                const FaceSide& previousSide =
                    sides[(index + sides.size() - 1) % sides.size()];
                const FaceSide& nextSide =
                    sides[index];

                const bool previousSelected =
                    contains_handle(targetEdges, previousSide.edge);
                const bool nextSelected =
                    contains_handle(targetEdges, nextSide.edge);

                if (!previousSelected && !nextSelected) {
                    if (contains_handle(targetVertices, nextSide.from)) {
                        const geometry::VertexHandle previousCut =
                            find_cut_at_vertex_toward(
                                edgeFaceCuts,
                                nextSide.from,
                                previousSide.from);
                        const geometry::VertexHandle nextCut =
                            find_cut_at_vertex_toward(
                                edgeFaceCuts,
                                nextSide.from,
                                nextSide.to);

                        if (mesh.is_valid(previousCut) &&
                            mesh.is_valid(nextCut) &&
                            previousCut != nextCut) {
                            rebuiltVertices.push_back(previousCut);
                            rebuiltVertices.push_back(nextCut);
                            continue;
                        }
                    }

                    rebuiltVertices.push_back(nextSide.from);
                    continue;
                }

                if (previousSelected) {
                    const EdgeFaceCut* cut =
                        find_edge_face_cut(
                            edgeFaceCuts,
                            face,
                            previousSide.edge);

                    if (cut != nullptr &&
                        mesh.is_valid(cut->endCut)) {
                        rebuiltVertices.push_back(cut->endCut);
                    }
                }

                if (nextSelected) {
                    const EdgeFaceCut* cut =
                        find_edge_face_cut(
                            edgeFaceCuts,
                            face,
                            nextSide.edge);

                    if (cut != nullptr &&
                        mesh.is_valid(cut->startCut)) {
                        rebuiltVertices.push_back(cut->startCut);
                    }
                }
            }

            if (!has_effective_area(mesh, rebuiltVertices)) {
                continue;
            }

            if (!editor.remove_face(face)) {
                continue;
            }

            const geometry::FaceHandle rebuiltFace =
                editor.add_face(rebuiltVertices);

            if (mesh.is_valid(rebuiltFace)) {
                ++createdFaceCount;
            }
        }

        for (const geometry::EdgeHandle edge : targetEdges) {
            std::vector<EdgeFaceCut> cuts;

            for (const EdgeFaceCut& cut : edgeFaceCuts) {
                if (cut.edge == edge) {
                    cuts.push_back(cut);
                }
            }

            if (cuts.size() == 1) {
                continue;
            }

            if (cuts.size() != 2) {
                return OperationResult::no_change(
                    "Bevel operation encountered a non-manifold edge region.");
            }

            const std::vector<geometry::VertexHandle> bevelFace =
                oriented_face_vertices(
                    mesh,
                    std::vector<geometry::VertexHandle>{
                cuts[0].cutAtA,
                cuts[0].cutAtB,
                cuts[1].cutAtB,
                cuts[1].cutAtA
                    },
                    normalized_or_zero(
                        cuts[0].faceNormal + cuts[1].faceNormal));

            if (!has_effective_area(mesh, bevelFace)) {
                continue;
            }

            const geometry::FaceHandle face =
                editor.add_face(bevelFace);

            if (mesh.is_valid(face)) {
                ++createdFaceCount;
            }
        }

        cleanup_loose_original_topology(
            mesh,
            editor,
            originalEdges,
            originalVertices);

        if (createdFaceCount == 0) {
            return OperationResult::no_change(
                "Bevel operation did not modify the mesh.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(
            editor.take_diff());
    }

    std::vector<geometry::EdgeHandle> BevelOp::collect_edges(
        const geometry::LEM& mesh) const
    {
        std::vector<geometry::EdgeHandle> result;

        if (!edges_.empty()) {
            result.reserve(edges_.size());

            for (geometry::EdgeHandle edge : edges_) {
                if (!mesh.is_valid(edge) || contains(result, edge)) {
                    continue;
                }

                result.push_back(edge);
            }

            return result;
        }

        if (target_ == BevelTarget::SelectedVertices) {
            for (const geometry::VertexHandle vertex : collect_vertices(mesh, {})) {
                for (const geometry::EdgeHandle edge :
                    geometry::TopologyTraversal::vertex_edges(mesh, vertex)) {
                    if (mesh.is_valid(edge) &&
                        mesh.edge(edge).selected &&
                        !contains(result, edge)) {
                        result.push_back(edge);
                    }
                }
            }

            return result;
        }

        const std::vector<geometry::EdgeHandle> activeEdges =
            geometry::TopologyTraversal::edges(mesh);

        result.reserve(activeEdges.size());

        for (geometry::EdgeHandle edge : activeEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            if (target_ == BevelTarget::SelectedEdges &&
                !mesh.edge(edge).selected) {
                continue;
            }

            result.push_back(edge);
        }

        return result;
    }

    std::vector<geometry::VertexHandle> BevelOp::collect_vertices(
        const geometry::LEM& mesh,
        const std::vector<geometry::EdgeHandle>& targetEdges) const
    {
        std::vector<geometry::VertexHandle> result;

        if (!vertices_.empty()) {
            result.reserve(vertices_.size());

            for (geometry::VertexHandle vertex : vertices_) {
                if (!mesh.is_valid(vertex) || contains(result, vertex)) {
                    continue;
                }

                result.push_back(vertex);
            }

            return result;
        }

        if (target_ == BevelTarget::SelectedVertices &&
            targetEdges.empty()) {
            const std::vector<geometry::VertexHandle> activeVertices =
                geometry::TopologyTraversal::vertices(mesh);

            result.reserve(activeVertices.size());

            for (geometry::VertexHandle vertex : activeVertices) {
                if (!mesh.is_valid(vertex) || !mesh.vertex(vertex).selected) {
                    continue;
                }

                result.push_back(vertex);
            }

            return result;
        }

        for (geometry::EdgeHandle edge : targetEdges) {
            if (!mesh.is_valid(edge)) {
                continue;
            }

            const std::array<geometry::VertexHandle, 2> vertices =
                geometry::TopologyTraversal::edge_vertices(mesh, edge);

            for (const geometry::VertexHandle vertex : vertices) {
                if (mesh.is_valid(vertex) && !contains(result, vertex)) {
                    result.push_back(vertex);
                }
            }
        }

        return result;
    }

    std::vector<geometry::FaceHandle> BevelOp::collect_faces(
        const geometry::LEM& mesh,
        const std::vector<geometry::VertexHandle>& targetVertices) const
    {
        std::vector<geometry::FaceHandle> result;

        for (geometry::VertexHandle vertex : targetVertices) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            const std::vector<geometry::FaceHandle> vertexFaces =
                geometry::TopologyTraversal::vertex_faces(mesh, vertex);

            for (geometry::FaceHandle face : vertexFaces) {
                if (!mesh.is_valid(face) || contains(result, face)) {
                    continue;
                }

                result.push_back(face);
            }
        }

        return result;
    }

    bool BevelOp::bevel_face(
        geometry::LEM& mesh,
        geometry::LEMEditor& editor,
        geometry::FaceHandle face,
        const std::vector<geometry::VertexHandle>& targetVertices) const
    {
        (void)editor;

        if (!mesh.is_valid(face)) {
            return false;
        }

        const std::vector<geometry::VertexHandle> sourceVertices =
            geometry::TopologyTraversal::face_vertices(mesh, face);

        if (sourceVertices.size() < 3) {
            return false;
        }

        for (geometry::VertexHandle vertex : sourceVertices) {
            if (contains(targetVertices, vertex)) {
                return true;
            }
        }

        return false;
    }

} // namespace locus::kernel::modeling
