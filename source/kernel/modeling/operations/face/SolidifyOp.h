/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <string_view>
#include <vector>

namespace locus::kernel::geometry {

    class LEM;
    class LEMEditor;

}

namespace locus::kernel::modeling {

    /**
     * @brief Source used by SolidifyOp to choose affected faces.
     */
    enum class SolidifyTarget {
        /**
         * @brief Solidify all active faces when no explicit face list is set.
         */
        Faces,

        /**
         * @brief Solidify only selected active faces when no explicit face list is set.
         */
        SelectedFaces
    };

    /**
     * @brief Direction mode used by SolidifyOp.
     */
    enum class SolidifyDirectionMode {
        /**
         * @brief Offset duplicated vertices along averaged face normals.
         */
        VertexNormals,

        /**
         * @brief Offset every duplicated vertex using the same explicit vector.
         */
        ExplicitOffset
    };

    /**
     * @brief Creates thickness from one or more editable mesh faces.
     *
     * SolidifyOp duplicates the target region, offsets the duplicated vertices and
     * creates rim faces along the boundary of the region.
     */
    class SolidifyOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty solidify operation.
         */
        SolidifyOp() = default;

        /**
         * @brief Creates an operation that solidifies one face.
         *
         * @param face Face to solidify.
         * @param thickness Offset distance.
         */
        SolidifyOp(geometry::FaceHandle face, float thickness);

        /**
         * @brief Creates an operation that solidifies explicit faces.
         *
         * @param faces Faces to solidify.
         * @param thickness Offset distance.
         */
        SolidifyOp(std::vector<geometry::FaceHandle> faces, float thickness);

        /**
         * @brief Creates an operation that solidifies one face using an explicit offset.
         *
         * @param face Face to solidify.
         * @param offset Offset applied to duplicated vertices.
         */
        SolidifyOp(geometry::FaceHandle face, const glm::vec3& offset);

        /**
         * @brief Creates an operation that solidifies explicit faces using an explicit offset.
         *
         * @param faces Faces to solidify.
         * @param offset Offset applied to duplicated vertices.
         */
        SolidifyOp(std::vector<geometry::FaceHandle> faces, const glm::vec3& offset);

        /**
         * @brief Creates an operation configured to solidify selected faces.
         *
         * @param thickness Offset distance.
         * @return Solidify operation.
         */
        [[nodiscard]] static SolidifyOp selected(float thickness);

        /**
         * @brief Creates an operation configured to solidify selected faces using an explicit offset.
         *
         * @param offset Offset applied to duplicated vertices.
         * @return Solidify operation.
         */
        [[nodiscard]] static SolidifyOp selected(const glm::vec3& offset);

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets how target faces are collected when no explicit list exists.
         *
         * @param target Target collection mode.
         */
        void set_target(SolidifyTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] SolidifyTarget target() const;

        /**
         * @brief Sets the direction mode.
         *
         * @param mode Direction mode.
         */
        void set_direction_mode(SolidifyDirectionMode mode);

        /**
         * @brief Returns the direction mode.
         *
         * @return Current direction mode.
         */
        [[nodiscard]] SolidifyDirectionMode direction_mode() const;

        /**
         * @brief Sets the normal offset thickness and switches to VertexNormals mode.
         *
         * @param thickness Offset distance.
         */
        void set_thickness(float thickness);

        /**
         * @brief Returns the normal offset thickness.
         *
         * @return Offset distance.
         */
        [[nodiscard]] float thickness() const;

        /**
         * @brief Sets the explicit offset and switches to ExplicitOffset mode.
         *
         * @param offset Offset applied to duplicated vertices.
         */
        void set_offset(const glm::vec3& offset);

        /**
         * @brief Returns the explicit offset.
         *
         * @return Offset vector.
         */
        [[nodiscard]] const glm::vec3& offset() const;

        /**
         * @brief Replaces the explicit face target list.
         *
         * @param faces Faces to solidify.
         */
        void set_faces(std::vector<geometry::FaceHandle> faces);

        /**
         * @brief Returns the explicit face target list.
         *
         * @return Read-only face list.
         */
        [[nodiscard]] const std::vector<geometry::FaceHandle>& faces() const;

        /**
         * @brief Clears the explicit face target list.
         */
        void clear_faces();

        /**
         * @brief Sets whether original source faces should remain.
         *
         * @param keepSourceFaces True to preserve source faces.
         */
        void set_keep_source_faces(bool keepSourceFaces);

        /**
         * @brief Checks whether original source faces remain.
         *
         * @return True when source faces are preserved.
         */
        [[nodiscard]] bool keep_source_faces() const;

        /**
         * @brief Sets whether offset cap faces should be created.
         *
         * @param createCaps True to create offset cap faces.
         */
        void set_create_caps(bool createCaps);

        /**
         * @brief Checks whether offset cap faces are created.
         *
         * @return True when offset cap creation is enabled.
         */
        [[nodiscard]] bool create_caps() const;

        /**
         * @brief Sets whether side faces should be created on region boundaries.
         *
         * @param createRims True to create boundary side faces.
         */
        void set_create_rims(bool createRims);

        /**
         * @brief Checks whether boundary side faces are created.
         *
         * @return True when rim creation is enabled.
         */
        [[nodiscard]] bool create_rims() const;

        /**
         * @brief Sets whether generated cap winding should be reversed.
         *
         * @param flipCaps True to reverse generated cap face winding.
         */
        void set_flip_caps(bool flipCaps);

        /**
         * @brief Checks whether generated cap winding is reversed.
         *
         * @return True when cap winding reversal is enabled.
         */
        [[nodiscard]] bool flip_caps() const;

        /**
         * @brief Sets whether generated rim winding should be reversed.
         *
         * @param flipRims True to reverse generated rim face winding.
         */
        void set_flip_rims(bool flipRims);

        /**
         * @brief Checks whether generated rim winding is reversed.
         *
         * @return True when rim winding reversal is enabled.
         */
        [[nodiscard]] bool flip_rims() const;

    private:
        /**
         * @brief Stores one source-to-duplicate vertex mapping.
         */
        struct VertexDuplicate {
            geometry::VertexHandle source{};
            geometry::VertexHandle duplicate{};
        };

        /**
         * @brief Stores one directed boundary edge of the solidified region.
         */
        struct BoundaryEdge {
            geometry::VertexHandle a{};
            geometry::VertexHandle b{};
        };

        /**
         * @brief Executes the solidify operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Collects valid face targets.
         *
         * @param mesh Mesh used to validate and query handles.
         * @return Face targets.
         */
        [[nodiscard]] std::vector<geometry::FaceHandle> collect_faces(
            const geometry::LEM& mesh) const;

        /**
         * @brief Collects all unique vertices used by target faces.
         *
         * @param mesh Mesh used to inspect face vertices.
         * @param faces Target faces.
         * @return Unique source vertices.
         */
        [[nodiscard]] std::vector<geometry::VertexHandle> collect_region_vertices(
            const geometry::LEM& mesh,
            const std::vector<geometry::FaceHandle>& faces) const;

        /**
         * @brief Collects directed boundary edges of the target face region.
         *
         * @param mesh Mesh used to inspect face vertices.
         * @param faces Target faces.
         * @return Boundary edges.
         */
        [[nodiscard]] std::vector<BoundaryEdge> collect_boundary_edges(
            const geometry::LEM& mesh,
            const std::vector<geometry::FaceHandle>& faces) const;

        /**
         * @brief Creates duplicate offset vertices.
         *
         * @param mesh Mesh used to read source positions.
         * @param editor Editor facade used to mutate the mesh.
         * @param faces Target faces used to compute averaged normals.
         * @param vertices Source vertices to duplicate.
         * @return Source-to-duplicate mapping.
         */
        [[nodiscard]] std::vector<VertexDuplicate> create_offset_vertices(
            const geometry::LEM& mesh,
            geometry::LEMEditor& editor,
            const std::vector<geometry::FaceHandle>& faces,
            const std::vector<geometry::VertexHandle>& vertices) const;

        /**
         * @brief Computes the offset vector for one source vertex.
         *
         * @param mesh Mesh used to read geometry.
         * @param faces Target faces used to compute averaged normals.
         * @param vertex Source vertex.
         * @return Offset vector.
         */
        [[nodiscard]] glm::vec3 offset_for_vertex(
            const geometry::LEM& mesh,
            const std::vector<geometry::FaceHandle>& faces,
            geometry::VertexHandle vertex) const;

        /**
         * @brief Computes a normalized face normal from the current vertex cycle.
         *
         * @param mesh Mesh used to read geometry.
         * @param face Face handle.
         * @return Normalized face normal, or zero when invalid.
         */
        [[nodiscard]] static glm::vec3 compute_face_normal(
            const geometry::LEM& mesh,
            geometry::FaceHandle face);

        /**
         * @brief Finds a duplicated vertex for a source vertex.
         *
         * @param duplicates Source-to-duplicate mapping.
         * @param source Source vertex.
         * @return Duplicated vertex, or an invalid handle when not found.
         */
        [[nodiscard]] static geometry::VertexHandle find_duplicate(
            const std::vector<VertexDuplicate>& duplicates,
            geometry::VertexHandle source);

        /**
         * @brief Checks whether a face handle already exists in a vector.
         *
         * @param handles Handle vector.
         * @param handle Handle to find.
         * @return True when the handle exists.
         */
        [[nodiscard]] static bool contains_face(
            const std::vector<geometry::FaceHandle>& handles,
            geometry::FaceHandle handle);

        /**
         * @brief Checks whether a vertex handle already exists in a vector.
         *
         * @param handles Handle vector.
         * @param handle Handle to find.
         * @return True when the handle exists.
         */
        [[nodiscard]] static bool contains_vertex(
            const std::vector<geometry::VertexHandle>& handles,
            geometry::VertexHandle handle);

        /**
         * @brief Checks whether two boundary edges are opposites.
         *
         * @param first First directed edge.
         * @param second Second directed edge.
         * @return True when the two edges have opposite direction.
         */
        [[nodiscard]] static bool opposite_edges(
            const BoundaryEdge& first,
            const BoundaryEdge& second);

        SolidifyTarget target_ = SolidifyTarget::Faces;
        SolidifyDirectionMode directionMode_ = SolidifyDirectionMode::VertexNormals;
        std::vector<geometry::FaceHandle> faces_{};
        float thickness_ = 0.1f;
        glm::vec3 offset_{ 0.0f, 0.0f, 0.1f };
        bool keepSourceFaces_ = true;
        bool createCaps_ = true;
        bool createRims_ = true;
        bool flipCaps_ = false;
        bool flipRims_ = false;
    };

}