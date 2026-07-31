/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kernel/modeling/operations/face/SolidifyOp.h"

#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/topology/TopologyTraversal.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace locus::kernel::modeling {

    namespace {

        constexpr float solidifyOffsetEpsilon =
            0.000001f;

    } // namespace

    SolidifyOp::SolidifyOp(geometry::FaceHandle face, float thickness)
        : faces_({ face })
        , thickness_(thickness) {
    }

    SolidifyOp::SolidifyOp(std::vector<geometry::FaceHandle> faces, float thickness)
        : faces_(std::move(faces))
        , thickness_(thickness) {
    }

    SolidifyOp::SolidifyOp(geometry::FaceHandle face, const glm::vec3& offset)
        : directionMode_(SolidifyDirectionMode::ExplicitOffset)
        , faces_({ face })
        , offset_(offset) {
    }

    SolidifyOp::SolidifyOp(std::vector<geometry::FaceHandle> faces, const glm::vec3& offset)
        : directionMode_(SolidifyDirectionMode::ExplicitOffset)
        , faces_(std::move(faces))
        , offset_(offset) {
    }

    SolidifyOp SolidifyOp::selected(float thickness) {
        SolidifyOp op;
        op.set_target(SolidifyTarget::SelectedFaces);
        op.set_thickness(thickness);
        return op;
    }

    SolidifyOp SolidifyOp::selected(const glm::vec3& offset) {
        SolidifyOp op;
        op.set_target(SolidifyTarget::SelectedFaces);
        op.set_offset(offset);
        return op;
    }

    std::string_view SolidifyOp::name() const {
        return "SolidifyOp";
    }

    void SolidifyOp::set_target(SolidifyTarget target) {
        target_ = target;
    }

    SolidifyTarget SolidifyOp::target() const {
        return target_;
    }

    void SolidifyOp::set_direction_mode(SolidifyDirectionMode mode) {
        directionMode_ = mode;
    }

    SolidifyDirectionMode SolidifyOp::direction_mode() const {
        return directionMode_;
    }

    void SolidifyOp::set_thickness(float thickness) {
        directionMode_ = SolidifyDirectionMode::VertexNormals;
        thickness_ = thickness;
    }

    float SolidifyOp::thickness() const {
        return thickness_;
    }

    void SolidifyOp::set_offset(const glm::vec3& offset) {
        directionMode_ = SolidifyDirectionMode::ExplicitOffset;
        offset_ = offset;
    }

    const glm::vec3& SolidifyOp::offset() const {
        return offset_;
    }

    void SolidifyOp::set_faces(std::vector<geometry::FaceHandle> faces) {
        faces_ = std::move(faces);
    }

    const std::vector<geometry::FaceHandle>& SolidifyOp::faces() const {
        return faces_;
    }

    void SolidifyOp::clear_faces() {
        faces_.clear();
    }

    void SolidifyOp::set_keep_source_faces(bool keepSourceFaces) {
        keepSourceFaces_ = keepSourceFaces;
    }

    bool SolidifyOp::keep_source_faces() const {
        return keepSourceFaces_;
    }

    void SolidifyOp::set_create_caps(bool createCaps) {
        createCaps_ = createCaps;
    }

    bool SolidifyOp::create_caps() const {
        return createCaps_;
    }

    void SolidifyOp::set_create_rims(bool createRims) {
        createRims_ = createRims;
    }

    bool SolidifyOp::create_rims() const {
        return createRims_;
    }

    void SolidifyOp::set_flip_caps(bool flipCaps) {
        flipCaps_ = flipCaps;
    }

    bool SolidifyOp::flip_caps() const {
        return flipCaps_;
    }

    void SolidifyOp::set_flip_rims(bool flipRims) {
        flipRims_ = flipRims;
    }

    bool SolidifyOp::flip_rims() const {
        return flipRims_;
    }

    OperationResult SolidifyOp::execute_impl(OperationContext& context) {
        geometry::LEM& mesh = context.editable_mesh();

        const std::vector<geometry::FaceHandle> targets = collect_faces(mesh);
        if (targets.empty()) {
            return OperationResult::no_change(
                "Solidify operation found no valid faces.");
        }

        if (!createCaps_ && !createRims_) {
            return OperationResult::no_change(
                "Solidify operation has both caps and rims disabled.");
        }

        if (directionMode_ == SolidifyDirectionMode::VertexNormals &&
            std::abs(thickness_) <= solidifyOffsetEpsilon) {
            return OperationResult::no_change(
                "Solidify operation has zero thickness.");
        }

        if (directionMode_ == SolidifyDirectionMode::ExplicitOffset &&
            glm::dot(offset_, offset_) <=
            solidifyOffsetEpsilon * solidifyOffsetEpsilon) {
            return OperationResult::no_change(
                "Solidify operation has zero offset.");
        }

        const std::vector<geometry::VertexHandle> sourceVertices =
            collect_region_vertices(mesh, targets);

        if (sourceVertices.empty()) {
            return OperationResult::no_change(
                "Solidify operation found no valid region vertices.");
        }

        geometry::LEMEditor editor(mesh);

        const std::vector<VertexDuplicate> duplicates =
            create_offset_vertices(mesh, editor, targets, sourceVertices);

        if (duplicates.size() != sourceVertices.size()) {
            return OperationResult::fail(
                kernel::ErrorCode::InvalidState,
                "Solidify operation failed to create all duplicated vertices.");
        }

        std::size_t createdFaces = 0;

        if (createCaps_) {
            for (geometry::FaceHandle face : targets) {
                if (!mesh.is_valid(face)) {
                    continue;
                }

                const std::vector<geometry::VertexHandle> faceVertices =
                    geometry::TopologyTraversal::face_vertices(mesh, face);

                if (faceVertices.size() < 3) {
                    continue;
                }

                std::vector<geometry::VertexHandle> capVertices;
                capVertices.reserve(faceVertices.size());

                for (geometry::VertexHandle sourceVertex : faceVertices) {
                    const geometry::VertexHandle duplicate =
                        find_duplicate(duplicates, sourceVertex);

                    if (!mesh.is_valid(duplicate)) {
                        capVertices.clear();
                        break;
                    }

                    capVertices.push_back(duplicate);
                }

                if (capVertices.size() < 3) {
                    continue;
                }

                if (flipCaps_) {
                    std::reverse(capVertices.begin(), capVertices.end());
                }

                const geometry::FaceHandle capFace = editor.add_face(capVertices);
                if (mesh.is_valid(capFace)) {
                    ++createdFaces;
                }
            }
        }

        if (createRims_) {
            const std::vector<BoundaryEdge> boundaryEdges =
                collect_boundary_edges(mesh, targets);

            for (const BoundaryEdge& boundary : boundaryEdges) {
                const geometry::VertexHandle duplicateA =
                    find_duplicate(duplicates, boundary.a);
                const geometry::VertexHandle duplicateB =
                    find_duplicate(duplicates, boundary.b);

                if (!mesh.is_valid(boundary.a) ||
                    !mesh.is_valid(boundary.b) ||
                    !mesh.is_valid(duplicateA) ||
                    !mesh.is_valid(duplicateB)) {
                    continue;
                }

                std::vector<geometry::VertexHandle> rimVertices;

                if (flipRims_) {
                    rimVertices = {
                        boundary.a,
                        duplicateA,
                        duplicateB,
                        boundary.b
                    };
                }
                else {
                    rimVertices = {
                        boundary.a,
                        boundary.b,
                        duplicateB,
                        duplicateA
                    };
                }

                const geometry::FaceHandle rimFace = editor.add_face(rimVertices);
                if (mesh.is_valid(rimFace)) {
                    ++createdFaces;
                }
            }
        }

        if (!keepSourceFaces_) {
            for (geometry::FaceHandle face : targets) {
                if (mesh.is_valid(face)) {
                    editor.remove_face(face);
                }
            }
        }

        if (createdFaces == 0) {
            return OperationResult::no_change(
                "Solidify operation did not create any face.");
        }

        if (context.rebuildNormals) {
            editor.rebuild_face_normals();
        }

        return OperationResult::success(editor.take_diff());
    }

    std::vector<geometry::FaceHandle> SolidifyOp::collect_faces(
        const geometry::LEM& mesh) const {
        std::vector<geometry::FaceHandle> result;

        if (!faces_.empty()) {
            result.reserve(faces_.size());

            for (geometry::FaceHandle face : faces_) {
                if (!mesh.is_valid(face) || contains_face(result, face)) {
                    continue;
                }

                result.push_back(face);
            }

            return result;
        }

        const std::vector<geometry::FaceHandle> activeFaces =
            geometry::TopologyTraversal::faces(mesh);

        result.reserve(activeFaces.size());

        for (geometry::FaceHandle face : activeFaces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            if (target_ == SolidifyTarget::SelectedFaces && !mesh.face(face).selected) {
                continue;
            }

            result.push_back(face);
        }

        return result;
    }

    std::vector<geometry::VertexHandle> SolidifyOp::collect_region_vertices(
        const geometry::LEM& mesh,
        const std::vector<geometry::FaceHandle>& faces) const {
        std::vector<geometry::VertexHandle> result;

        for (geometry::FaceHandle face : faces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            const std::vector<geometry::VertexHandle> faceVertices =
                geometry::TopologyTraversal::face_vertices(mesh, face);

            for (geometry::VertexHandle vertex : faceVertices) {
                if (!mesh.is_valid(vertex) || contains_vertex(result, vertex)) {
                    continue;
                }

                result.push_back(vertex);
            }
        }

        return result;
    }

    std::vector<SolidifyOp::BoundaryEdge> SolidifyOp::collect_boundary_edges(
        const geometry::LEM& mesh,
        const std::vector<geometry::FaceHandle>& faces) const {
        std::vector<BoundaryEdge> boundaries;

        for (geometry::FaceHandle face : faces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            const std::vector<geometry::VertexHandle> faceVertices =
                geometry::TopologyTraversal::face_vertices(mesh, face);

            if (faceVertices.size() < 3) {
                continue;
            }

            for (std::size_t i = 0; i < faceVertices.size(); ++i) {
                const std::size_t next = (i + 1) % faceVertices.size();

                BoundaryEdge candidate{
                    faceVertices[i],
                    faceVertices[next]
                };

                bool removedInternalEdge = false;

                for (auto it = boundaries.begin(); it != boundaries.end(); ++it) {
                    if (opposite_edges(candidate, *it)) {
                        boundaries.erase(it);
                        removedInternalEdge = true;
                        break;
                    }
                }

                if (!removedInternalEdge) {
                    boundaries.push_back(candidate);
                }
            }
        }

        return boundaries;
    }

    std::vector<SolidifyOp::VertexDuplicate> SolidifyOp::create_offset_vertices(
        const geometry::LEM& mesh,
        geometry::LEMEditor& editor,
        const std::vector<geometry::FaceHandle>& faces,
        const std::vector<geometry::VertexHandle>& vertices) const {
        std::vector<VertexDuplicate> duplicates;
        duplicates.reserve(vertices.size());

        for (geometry::VertexHandle vertex : vertices) {
            if (!mesh.is_valid(vertex)) {
                continue;
            }

            const glm::vec3 sourcePosition = mesh.vertex(vertex).position;
            const glm::vec3 vertexOffset = offset_for_vertex(mesh, faces, vertex);

            if (!std::isfinite(vertexOffset.x) ||
                !std::isfinite(vertexOffset.y) ||
                !std::isfinite(vertexOffset.z)) {
                continue;
            }

            if (glm::dot(vertexOffset, vertexOffset) <= 0.0f) {
                continue;
            }

            const geometry::VertexHandle duplicate =
                editor.add_vertex(sourcePosition + vertexOffset);

            if (!mesh.is_valid(duplicate)) {
                continue;
            }

            duplicates.push_back(VertexDuplicate{
                vertex,
                duplicate
                });
        }

        return duplicates;
    }

    glm::vec3 SolidifyOp::offset_for_vertex(
        const geometry::LEM& mesh,
        const std::vector<geometry::FaceHandle>& faces,
        geometry::VertexHandle vertex) const {
        if (directionMode_ == SolidifyDirectionMode::ExplicitOffset) {
            return offset_;
        }

        glm::vec3 normalSum{ 0.0f, 0.0f, 0.0f };

        for (geometry::FaceHandle face : faces) {
            if (!mesh.is_valid(face)) {
                continue;
            }

            const std::vector<geometry::VertexHandle> faceVertices =
                geometry::TopologyTraversal::face_vertices(mesh, face);

            if (!contains_vertex(faceVertices, vertex)) {
                continue;
            }

            normalSum += compute_face_normal(mesh, face);
        }

        const float lengthSquared = glm::dot(normalSum, normalSum);
        if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        return normalSum * (thickness_ / std::sqrt(lengthSquared));
    }

    glm::vec3 SolidifyOp::compute_face_normal(
        const geometry::LEM& mesh,
        geometry::FaceHandle face) {
        if (!mesh.is_valid(face)) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        const std::vector<geometry::VertexHandle> vertices =
            geometry::TopologyTraversal::face_vertices(mesh, face);

        if (vertices.size() < 3) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        glm::vec3 normal{ 0.0f, 0.0f, 0.0f };

        for (std::size_t i = 0; i < vertices.size(); ++i) {
            const glm::vec3& a = mesh.vertex(vertices[i]).position;
            const glm::vec3& b = mesh.vertex(vertices[(i + 1) % vertices.size()]).position;

            normal += glm::cross(a, b);
        }

        const float lengthSquared = glm::dot(normal, normal);
        if (!std::isfinite(lengthSquared) || lengthSquared <= 0.0f) {
            return glm::vec3{ 0.0f, 0.0f, 0.0f };
        }

        return normal / std::sqrt(lengthSquared);
    }

    geometry::VertexHandle SolidifyOp::find_duplicate(
        const std::vector<VertexDuplicate>& duplicates,
        geometry::VertexHandle source) {
        for (const VertexDuplicate& duplicate : duplicates) {
            if (duplicate.source == source) {
                return duplicate.duplicate;
            }
        }

        return {};
    }

    bool SolidifyOp::contains_face(
        const std::vector<geometry::FaceHandle>& handles,
        geometry::FaceHandle handle) {
        return std::find(handles.begin(), handles.end(), handle) != handles.end();
    }

    bool SolidifyOp::contains_vertex(
        const std::vector<geometry::VertexHandle>& handles,
        geometry::VertexHandle handle) {
        return std::find(handles.begin(), handles.end(), handle) != handles.end();
    }

    bool SolidifyOp::opposite_edges(
        const BoundaryEdge& first,
        const BoundaryEdge& second) {
        return first.a == second.b && first.b == second.a;
    }

}
