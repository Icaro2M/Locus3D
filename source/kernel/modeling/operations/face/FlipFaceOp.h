#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

    enum class FlipFaceTarget {
        Faces,
        SelectedFaces
    };

    class FlipFaceOp final : public IOperation {
    public:
        FlipFaceOp() = default;

        explicit FlipFaceOp(std::vector<geometry::FaceHandle> faces);

        [[nodiscard]] std::string_view name() const override;

        void set_target(FlipFaceTarget target);

        [[nodiscard]] FlipFaceTarget target() const;

        void set_faces(std::vector<geometry::FaceHandle> faces);

        [[nodiscard]] const std::vector<geometry::FaceHandle>& faces() const;

        void clear_faces();

    private:
        [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

        [[nodiscard]] std::vector<geometry::FaceHandle> collect_faces(const geometry::LEM& mesh) const;

        bool flip_face(geometry::LEM& mesh, geometry::FaceHandle faceHandle, geometry::LEMDiff& diff) const;

        FlipFaceTarget target_ = FlipFaceTarget::Faces;
        std::vector<geometry::FaceHandle> faces_{};
    };

}