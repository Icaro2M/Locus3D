/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <algorithm>
#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

    /**
     * @brief Source used by InsetFaceOp to choose affected faces.
     */
    enum class InsetFaceTarget {
        /**
         * @brief Inset all active faces when no explicit face list is set.
         */
        Faces,

        /**
         * @brief Inset only selected active faces when no explicit face list is set.
         */
        SelectedFaces
    };

    /**
     * @brief Insets editable mesh faces through the LEM editor facade.
     */
    class InsetFaceOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty inset operation.
         */
        InsetFaceOp() = default;

        /**
         * @brief Creates an operation that insets one face.
         *
         * @param face Face to inset.
         * @param factor Interpolation factor toward the face center.
         */
        InsetFaceOp(geometry::FaceHandle face, float factor);

        /**
         * @brief Creates an operation that insets explicit faces.
         *
         * @param faces Faces to inset.
         * @param factor Interpolation factor toward each face center.
         */
        InsetFaceOp(std::vector<geometry::FaceHandle> faces, float factor);

        /**
         * @brief Creates an operation configured to inset selected faces.
         *
         * @param factor Interpolation factor toward each selected face center.
         * @return Face inset operation.
         */
        [[nodiscard]] static InsetFaceOp selected(float factor);

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
        void set_target(InsetFaceTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] InsetFaceTarget target() const;

        /**
         * @brief Sets the inset interpolation factor.
         *
         * @param factor Interpolation factor toward the face center.
         */
        void set_factor(float factor);

        /**
         * @brief Returns the inset interpolation factor.
         *
         * @return Inset factor.
         */
        [[nodiscard]] float factor() const;

        /**
         * @brief Replaces the explicit face target list.
         *
         * @param faces Faces to inset.
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

    private:
        /**
         * @brief Executes the face inset operation.
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
         * @brief Insets one valid face.
         *
         * @param mesh Mesh being edited.
         * @param editor Editor facade used to mutate the mesh.
         * @param face Face to inset.
         * @return True when the face produced new geometry.
         */
        [[nodiscard]] bool inset_face(
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

        InsetFaceTarget target_ = InsetFaceTarget::Faces;
        std::vector<geometry::FaceHandle> faces_{};
        float factor_ = 0.2f;
    };

}