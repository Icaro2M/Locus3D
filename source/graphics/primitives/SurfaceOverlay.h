/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace locus::graphics
{
    /**
     * @brief One world-space vertex used by translucent surface overlays.
     */
    struct SurfaceOverlayVertex
    {
        /**
         * @brief Vertex position in world space.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Linear RGBA overlay color.
         */
        ColorRGBA color{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    /**
     * @brief CPU-side indexed triangle batch for surface overlays.
     */
    struct SurfaceOverlayBatch
    {
        /**
         * @brief Object-to-world transform applied by the renderer.
         */
        glm::mat4 modelMatrix{ 1.0f };

        /**
         * @brief Local-space vertex buffer.
         */
        std::vector<SurfaceOverlayVertex> vertices;

        /**
         * @brief Triangle index buffer. Every three indices form one triangle.
         */
        std::vector<std::uint32_t> indices;

        /**
         * @brief Checks whether the batch has drawable triangle data.
         *
         * @return True when no complete indexed triangles exist.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return vertices.empty() || indices.empty();
        }

        /**
         * @brief Returns the number of stored vertices.
         *
         * @return Vertex count.
         */
        [[nodiscard]] std::size_t vertex_count() const noexcept
        {
            return vertices.size();
        }

        /**
         * @brief Returns the number of stored indices.
         *
         * @return Index count.
         */
        [[nodiscard]] std::size_t index_count() const noexcept
        {
            return indices.size();
        }

        /**
         * @brief Returns the number of complete indexed triangles.
         *
         * @return Triangle count.
         */
        [[nodiscard]] std::size_t triangle_count() const noexcept
        {
            return indices.size() / 3u;
        }
    };

    /**
     * @brief Checks whether a surface overlay vertex is finite.
     *
     * @param vertex Vertex to validate.
     * @return True when the vertex can be submitted safely.
     */
    [[nodiscard]] inline bool is_drawable(const SurfaceOverlayVertex& vertex) noexcept
    {
        return std::isfinite(vertex.position.x) &&
            std::isfinite(vertex.position.y) &&
            std::isfinite(vertex.position.z) &&
            std::isfinite(vertex.color.r) &&
            std::isfinite(vertex.color.g) &&
            std::isfinite(vertex.color.b) &&
            std::isfinite(vertex.color.a) &&
            vertex.color.a > 0.0f;
    }

    /**
     * @brief Checks whether one indexed triangle is valid and non-degenerate.
     *
     * @param batch Batch containing the triangle.
     * @param offset First index of the triangle.
     * @return True when the triangle can be submitted safely.
     */
    [[nodiscard]] inline bool is_drawable_triangle(
        const SurfaceOverlayBatch& batch,
        std::size_t offset) noexcept
    {
        if (offset + 2u >= batch.indices.size()) {
            return false;
        }

        const std::uint32_t a = batch.indices[offset];
        const std::uint32_t b = batch.indices[offset + 1u];
        const std::uint32_t c = batch.indices[offset + 2u];

        if (a >= batch.vertices.size() ||
            b >= batch.vertices.size() ||
            c >= batch.vertices.size()) {
            return false;
        }

        const SurfaceOverlayVertex& va = batch.vertices[a];
        const SurfaceOverlayVertex& vb = batch.vertices[b];
        const SurfaceOverlayVertex& vc = batch.vertices[c];

        if (!is_drawable(va) || !is_drawable(vb) || !is_drawable(vc)) {
            return false;
        }

        const glm::vec3 ab = vb.position - va.position;
        const glm::vec3 ac = vc.position - va.position;
        return glm::dot(glm::cross(ab, ac), glm::cross(ab, ac)) > 0.0000000001f;
    }
}
