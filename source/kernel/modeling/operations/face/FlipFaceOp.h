/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <string_view>

namespace locus::kernel::modeling {

    /**
     * @brief Operation that flips the winding order of one editable mesh face.
     */
    class FlipFaceOp final : public IOperation {
    public:
        /**
         * @brief Creates an empty flip face operation.
         */
        FlipFaceOp() = default;

        /**
         * @brief Creates a flip face operation for a specific face.
         *
         * @param face Face to flip.
         */
        explicit FlipFaceOp(geometry::FaceHandle face);

        /**
         * @brief Returns the stable operation name.
         *
         * @return Operation name.
         */
        [[nodiscard]] std::string_view name() const override;

        /**
         * @brief Sets the face affected by the operation.
         *
         * @param face Face to flip.
         */
        void set_face(geometry::FaceHandle face);

        /**
         * @brief Returns the face affected by the operation.
         *
         * @return Face handle.
         */
        [[nodiscard]] geometry::FaceHandle face() const;

    private:
        /**
         * @brief Executes the flip face operation.
         *
         * @param context Operation execution context.
         * @return Operation result with the produced mesh diff.
         */
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        geometry::FaceHandle face_{};
    };

}