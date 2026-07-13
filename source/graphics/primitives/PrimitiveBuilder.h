/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/primitives/PrimitiveMesh.h"

#include <cstddef>

#include <glm/vec3.hpp>

namespace locus::graphics {

    /**
     * @brief Builds generic CPU-side point, line, or triangle geometry.
     *
     * Each PrimitiveBuilder instance targets exactly one primitive topology.
     * Convenience methods reject operations that are incompatible with the
     * configured topology.
     *
     * The builder produces non-indexed geometry. Indexed primitive construction
     * may be added separately if a concrete consumer requires vertex reuse.
     */
    class PrimitiveBuilder {
    public:
        /**
         * @brief Creates a builder for a primitive topology.
         *
         * The initial builder API supports Points, Lines, and Triangles.
         * Strip topologies are rejected by the convenience methods.
         *
         * @param topology Primitive topology produced by this builder.
         */
        explicit PrimitiveBuilder(
            PrimitiveTopology topology = PrimitiveTopology::Triangles
        );

        /**
         * @brief Clears all accumulated geometry.
         *
         * The configured primitive topology is preserved.
         */
        void clear();

        /**
         * @brief Adds one point.
         *
         * @param position Point position.
         * @param color Point vertex color.
         * @return True when the builder topology is Points.
         */
        bool add_point(
            const glm::vec3& position,
            const ColorRGBA& color = {}
        );

        /**
         * @brief Adds one line segment.
         *
         * @param start Segment start position.
         * @param end Segment end position.
         * @param color Vertex color applied to both endpoints.
         * @return True when the builder topology is Lines.
         */
        bool add_line(
            const glm::vec3& start,
            const glm::vec3& end,
            const ColorRGBA& color = {}
        );

        /**
         * @brief Adds one line segment with independent endpoint colors.
         *
         * @param start Segment start position.
         * @param end Segment end position.
         * @param startColor Start vertex color.
         * @param endColor End vertex color.
         * @return True when the builder topology is Lines.
         */
        bool add_line(
            const glm::vec3& start,
            const glm::vec3& end,
            const ColorRGBA& startColor,
            const ColorRGBA& endColor
        );

        /**
         * @brief Adds one triangle with a generated face normal.
         *
         * Degenerate triangles receive a zero normal.
         *
         * @param a First triangle position.
         * @param b Second triangle position.
         * @param c Third triangle position.
         * @param color Vertex color applied to the triangle.
         * @return True when the builder topology is Triangles.
         */
        bool add_triangle(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c,
            const ColorRGBA& color = {}
        );

        /**
         * @brief Adds one triangle with explicit per-vertex data.
         *
         * @param a First primitive vertex.
         * @param b Second primitive vertex.
         * @param c Third primitive vertex.
         * @return True when the builder topology is Triangles.
         */
        bool add_triangle(
            const PrimitiveVertex& a,
            const PrimitiveVertex& b,
            const PrimitiveVertex& c
        );

        /**
         * @brief Adds a quad as two triangles.
         *
         * The expected vertex order follows the quad boundary. The generated
         * triangles are (a, b, c) and (a, c, d).
         *
         * @param a First quad position.
         * @param b Second quad position.
         * @param c Third quad position.
         * @param d Fourth quad position.
         * @param color Vertex color applied to both triangles.
         * @return True when the builder topology is Triangles.
         */
        bool add_quad(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c,
            const glm::vec3& d,
            const ColorRGBA& color = {}
        );

        /**
         * @brief Adds the twelve edges of an axis-aligned box.
         *
         * Inverted bounds are normalized before the edge vertices are generated.
         *
         * @param minPoint First box corner.
         * @param maxPoint Opposite box corner.
         * @param color Vertex color applied to all edges.
         * @return True when the builder topology is Lines.
         */
        bool add_box_edges(
            const glm::vec3& minPoint,
            const glm::vec3& maxPoint,
            const ColorRGBA& color = {}
        );

        /**
         * @brief Checks whether no primitive geometry has been accumulated.
         *
         * @return True when the internal mesh has no vertices.
         */
        [[nodiscard]] bool is_empty() const;

        /**
         * @brief Returns the number of accumulated vertices.
         *
         * @return Current vertex count.
         */
        [[nodiscard]] std::size_t vertex_count() const;

        /**
         * @brief Returns the primitive topology produced by the builder.
         *
         * @return Configured primitive topology.
         */
        [[nodiscard]] PrimitiveTopology topology() const;

        /**
         * @brief Provides read-only access to the accumulated primitive mesh.
         *
         * @return Const reference to the internal mesh.
         */
        [[nodiscard]] const PrimitiveMesh& mesh() const;

        /**
         * @brief Moves the accumulated geometry out of the builder.
         *
         * The builder is cleared after the operation and retains its configured
         * primitive topology.
         *
         * @return Completed primitive mesh.
         */
        [[nodiscard]] PrimitiveMesh build();

    private:
        /**
         * @brief Creates a primitive vertex from graphical attributes.
         *
         * @param position Vertex position.
         * @param normal Vertex normal.
         * @param color Vertex color.
         * @return Constructed primitive vertex.
         */
        static PrimitiveVertex make_vertex(
            const glm::vec3& position,
            const glm::vec3& normal,
            const ColorRGBA& color
        );

        /**
         * @brief Computes a normalized triangle face normal.
         *
         * Degenerate triangles return a zero vector.
         *
         * @param a First triangle position.
         * @param b Second triangle position.
         * @param c Third triangle position.
         * @return Normalized face normal or zero.
         */
        static glm::vec3 triangle_normal(
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c
        );

    private:
        PrimitiveMesh mesh_{};
    };

} // namespace locus::graphics