/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMDiff.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/glm.hpp>

#include <vector>

namespace locus::kernel::geometry {

/**
 * @brief Mesh elements and diff produced by a primitive builder.
 */
struct PrimitiveBuildResult {
    /**
     * @brief Vertices created by the primitive build.
     */
    std::vector<VertexHandle> vertices{};
    /**
     * @brief Edges created by the primitive build.
     */
    std::vector<EdgeHandle> edges{};
    /**
     * @brief Faces created by the primitive build.
     */
    std::vector<FaceHandle> faces{};
    /**
     * @brief Mesh changes recorded during the build.
     */
    LEMDiff diff{};
    /**
     * @brief True when the primitive was fully created.
     */
    bool success = false;

    /**
     * @brief Converts the result to true when the build succeeded.
     */
    [[nodiscard]] explicit operator bool() const
    {
        return success;
    }

    /**
     * @brief Checks whether the build produced no mesh elements.
     *
     * @return True when all created element lists are empty.
     */
    [[nodiscard]] bool empty() const
    {
        return vertices.empty() && edges.empty() && faces.empty();
    }
};

/**
 * @brief Parameters used to create an axis-aligned box primitive.
 */
struct BoxParameters {
    /**
     * @brief Box center in object space.
     */
    glm::vec3 center{ 0.0f, 0.0f, 0.0f };
    /**
     * @brief Box dimensions along the local X, Y, and Z axes.
     */
    glm::vec3 size{ 1.0f, 1.0f, 1.0f };
    /**
     * @brief True when faces created by the builder should be selected.
     */
    bool selectCreatedFaces = false;

    /**
     * @brief Checks whether the box dimensions are usable.
     *
     * @return True when every size component is positive.
     */
    [[nodiscard]] bool is_valid() const
    {
        return size.x > 0.0f && size.y > 0.0f && size.z > 0.0f;
    }
};

}
