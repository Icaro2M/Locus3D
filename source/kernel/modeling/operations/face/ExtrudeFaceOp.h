/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <algorithm>
#include <glm/vec3.hpp>
#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

    /**
     * @brief Source used by ExtrudeFaceOp to choose affected faces.
     */
    enum class ExtrudeFaceTarget {
        /**
         * @brief Extrude all active faces when no explicit face list is set.
         */
        Faces,

        /**
         * @brief Extrude only selected active faces when no explicit face list is set.
         */
        SelectedFaces
    };

    /**
     * @brief Direction mode used by ExtrudeFaceOp.
     */
    enum class ExtrudeFaceDirectionMode {
        /**
         * @brief Extrude each face along its own computed normal.
         */
        FaceNormal,

        /**
         * @brief Extrude every target face using the same explicit offset vector.
         */
        ExplicitOffset
    };

    /**
     * @brief Extrudes editable mesh faces through the LEM editor facade.
     */
    class ExtrudeFaceOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty face extrusion operation.
         */
        ExtrudeFaceOp() = default;

        /**
         * @brief Creates an operation that extrudes one face along its normal.
         *
         * @param face Face to extrude.
         * @param distance Extrusion distance.
         */
        ExtrudeFaceOp(geometry::FaceHandle face, float distance);

        /**
         * @brief Creates an operation that extrudes explicit faces along their normals.
         *
         * @param faces Faces to extrude.
         * @param distance Extrusion distance.
         */
        ExtrudeFaceOp(std::vector<geometry::FaceHandle> faces, float distance);

        /**
         * @brief Creates an operation that extrudes one face using an explicit offset.
         *
         * @param face Face to extrude.
         * @param offset Offset applied to duplicated vertices.
         */
        ExtrudeFaceOp(geometry::FaceHandle face, const glm::vec3& offset);

        /**
         * @brief Creates an operation that extrudes explicit faces using an explicit offset.
         *
         * @param faces Faces to extrude.
         * @param offset Offset applied to duplicated vertices.
         */
        ExtrudeFaceOp(std::vector<geometry::FaceHandle> faces, const glm::vec3& offset);

        /**
         * @brief Creates an operation configured to extrude selected faces.
         *
         * @param distance Extrusion distance along each selected face normal.
         * @return Face extrusion operation.
         */
        [[nodiscard]] static ExtrudeFaceOp selected(float distance);

        /**
         * @brief Creates an operation configured to extrude selected faces using an explicit offset.
         *
         * @param offset Offset applied to duplicated vertices.
         * @return Face extrusion operation.
         */
        [[nodiscard]] static ExtrudeFaceOp selected(const glm::vec3& offset);

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
        void set_target(ExtrudeFaceTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] ExtrudeFaceTarget target() const;

        /**
         * @brief Sets the direction mode.
         *
         * @param mode Direction mode.
         */
        void set_direction_mode(ExtrudeFaceDirectionMode mode);

        /**
         * @brief Returns the direction mode.
         *
         * @return Current direction mode.
         */
        [[nodiscard]] ExtrudeFaceDirectionMode direction_mode() const;

        /**
         * @brief Sets the normal extrusion distance and switches to FaceNormal mode.
         *
         * @param distance Extrusion distance.
         */
        void set_distance(float distance);

        /**
         * @brief Returns the normal extrusion distance.
         *
         * @return Extrusion distance.
         */
        [[nodiscard]] float distance() const;

        /**
         * @brief Sets the explicit extrusion offset and switches to ExplicitOffset mode.
         *
         * @param offset Offset applied to duplicated vertices.
         */
        void set_offset(const glm::vec3& offset);

        /**
         * @brief Returns the explicit extrusion offset.
         *
         * @return Offset vector.
         */
        [[nodiscard]] const glm::vec3& offset() const;

        /**
         * @brief Replaces the explicit face target list.
         *
         * @param faces Faces to extrude.
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
         * @brief Sets whether the source face should remain after extrusion.
         *
         * @param keepSourceFace True to preserve the source face.
         */
        void set_keep_source_face(bool keepSourceFace);

        /**
         * @brief Checks whether the source face remains after extrusion.
         *
         * @return True when source faces are preserved.
         */
        [[nodiscard]] bool keep_source_face() const;

    private:
        /**
         * @brief Executes the face extrusion operation.
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
         * @brief Computes the extrusion offset for one face.
         *
         * @param mesh Mesh used to evaluate face geometry.
         * @param face Face being extruded.
         * @return Offset vector.
         */
        [[nodiscard]] glm::vec3 offset_for_face(
            const geometry::LEM& mesh,
            geometry::FaceHandle face) const;

        /**
         * @brief Extrudes one valid face.
         *
         * @param mesh Mesh being edited.
         * @param editor Editor facade used to mutate the mesh.
         * @param face Face to extrude.
         * @return True when the face produced new geometry.
         */
        [[nodiscard]] bool extrude_face(
            geometry::LEM& mesh,
            geometry::LEMEditor& editor,
            geometry::FaceHandle face) const;

        /**
         * @brief Checks whether a handle already exists in a vector.
         *
         * @param handles Handle vector.
         * @param handle Handle to find.
         * @return True when the handle exists.
         */
        template <typename Handle>
        [[nodiscard]] static bool contains(
            const std::vector<Handle>& handles,
            Handle handle)
        {
            return std::find(handles.begin(), handles.end(), handle) != handles.end();
        }

        ExtrudeFaceTarget target_ = ExtrudeFaceTarget::Faces;
        ExtrudeFaceDirectionMode directionMode_ = ExtrudeFaceDirectionMode::FaceNormal;
        std::vector<geometry::FaceHandle> faces_{};
        float distance_ = 1.0f;
        glm::vec3 offset_{ 0.0f, 1.0f, 0.0f };
        bool keepSourceFace_ = false;
    };

}