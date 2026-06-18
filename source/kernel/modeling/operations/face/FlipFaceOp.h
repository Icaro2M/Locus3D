/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

    /**
     * @brief Source used by FlipFaceOp to choose affected faces.
     */
    enum class FlipFaceTarget {
        /**
         * @brief Flip all active faces when no explicit face list is set.
         */
        Faces,
        /**
         * @brief Flip only selected active faces when no explicit face list is set.
         */
        SelectedFaces
    };

    /**
     * @brief Reverses face winding and recomputes face normals.
     */
    class FlipFaceOp final : public IOperation {
    public:
        FlipFaceOp() = default;

        /**
         * @brief Creates an operation with an explicit face list.
         *
         * @param faces Faces to flip.
         */
        explicit FlipFaceOp(std::vector<geometry::FaceHandle> faces);

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
        void set_target(FlipFaceTarget target);

        /**
         * @brief Returns the target collection mode.
         *
         * @return Current target mode.
         */
        [[nodiscard]] FlipFaceTarget target() const;

        /**
         * @brief Replaces the explicit face target list.
         *
         * @param faces Faces to flip.
         */
        void set_faces(std::vector<geometry::FaceHandle> faces);

        /**
         * @brief Returns the explicit face target list.
         *
         * @return Read-only target face list.
         */
        [[nodiscard]] const std::vector<geometry::FaceHandle>& faces() const;

        /**
         * @brief Clears the explicit face target list.
         */
        void clear_faces();

    private:
        /**
         * @brief Executes the face flip operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        /**
         * @brief Collects valid faces affected by this operation.
         *
         * @param mesh Mesh used to validate and query handles.
         * @return Target face list.
         */
        [[nodiscard]] std::vector<geometry::FaceHandle> collect_faces(const geometry::LEM& mesh) const;

        /**
         * @brief Reverses a single face boundary cycle.
         *
         * @param mesh Mesh containing the face.
         * @param faceHandle Face to flip.
         * @param diff Diff that receives loop, face, and normal changes.
         * @return True when the face was flipped.
         */
        bool flip_face(geometry::LEM& mesh, geometry::FaceHandle faceHandle, geometry::LEMDiff& diff) const;

        FlipFaceTarget target_ = FlipFaceTarget::Faces;
        std::vector<geometry::FaceHandle> faces_{};
    };

}
