/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"
#include "kernel/modeling/core/IOperation.h"

#include <glm/glm.hpp>

#include <string_view>
#include <vector>

namespace locus::kernel::modeling {

/**
 * @brief Source used by TransformOp to choose affected vertices.
 */
enum class TransformTarget {
    /**
     * @brief Transform all active vertices when no explicit vertex list is set.
     */
    Vertices,
    /**
     * @brief Transform only selected active vertices when no explicit vertex list is set.
     */
    SelectedVertices
};

/**
 * @brief Applies a matrix transform to editable mesh vertices.
 */
class TransformOp final : public IOperation {
public:
    TransformOp() = default;

    /**
     * @brief Creates an operation that transforms all target vertices.
     *
     * @param transform Transformation matrix applied to each vertex position.
     */
    explicit TransformOp(const glm::mat4& transform);

    /**
     * @brief Creates an operation with an explicit vertex list.
     *
     * @param vertices Vertices to transform.
     * @param transform Transformation matrix applied to each vertex position.
     */
    TransformOp(std::vector<geometry::VertexHandle> vertices, const glm::mat4& transform);

    /**
     * @brief Returns the stable operation name.
     *
     * @return Operation name.
     */
    [[nodiscard]] std::string_view name() const override;

    /**
     * @brief Sets the transformation matrix.
     *
     * @param transform Matrix applied to each target vertex.
     */
    void set_transform(const glm::mat4& transform);

    /**
     * @brief Returns the transformation matrix.
     *
     * @return Current transformation matrix.
     */
    [[nodiscard]] const glm::mat4& transform() const;

    /**
     * @brief Sets how target vertices are collected when no explicit list exists.
     *
     * @param target Target collection mode.
     */
    void set_target(TransformTarget target);

    /**
     * @brief Returns the target collection mode.
     *
     * @return Current target mode.
     */
    [[nodiscard]] TransformTarget target() const;

    /**
     * @brief Replaces the explicit vertex target list.
     *
     * @param vertices Vertices to transform.
     */
    void set_vertices(std::vector<geometry::VertexHandle> vertices);

    /**
     * @brief Returns the explicit vertex target list.
     *
     * @return Read-only target vertex list.
     */
    [[nodiscard]] const std::vector<geometry::VertexHandle>& vertices() const;

    /**
     * @brief Clears the explicit vertex target list.
     */
    void clear_vertices();

private:
    /**
     * @brief Executes the transform operation.
     *
     * @param context Operation execution context.
     * @return Operation result with the produced mesh diff.
     */
    [[nodiscard]] OperationResult execute_impl(OperationContext& context) override;

    /**
     * @brief Collects valid vertices affected by this operation.
     *
     * @param mesh Mesh used to validate and query handles.
     * @return Target vertex list.
     */
    [[nodiscard]] std::vector<geometry::VertexHandle> collect_vertices(const geometry::LEM& mesh) const;

    glm::mat4 transform_{ 1.0f };
    TransformTarget target_ = TransformTarget::Vertices;
    std::vector<geometry::VertexHandle> vertices_{};
};

}
