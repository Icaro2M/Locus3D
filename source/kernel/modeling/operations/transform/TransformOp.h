#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <glm/glm.hpp>

#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

enum class TransformTarget {
    Vertices,
    SelectedVertices
};

class TransformOp final : public IOperation {
public:
    TransformOp() = default;

    explicit TransformOp(const glm::mat4& transform);

    TransformOp(std::vector<geometry::VertexHandle> vertices, const glm::mat4& transform);

    [[nodiscard]] std::string_view name() const override;

    void set_transform(const glm::mat4& transform);

    [[nodiscard]] const glm::mat4& transform() const;

    void set_target(TransformTarget target);

    [[nodiscard]] TransformTarget target() const;

    void set_vertices(std::vector<geometry::VertexHandle> vertices);

    [[nodiscard]] const std::vector<geometry::VertexHandle>& vertices() const;

    void clear_vertices();

private:
    [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

    [[nodiscard]] std::vector<geometry::VertexHandle> collect_vertices(const geometry::LEM& mesh) const;

    glm::mat4 transform_{ 1.0f };
    TransformTarget target_ = TransformTarget::Vertices;
    std::vector<geometry::VertexHandle> vertices_{};
};

}