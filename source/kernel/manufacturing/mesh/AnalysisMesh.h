/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/manufacturing/mesh/MeshHandleMapping.h"
#include "kernel/math/Bounds.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Index type used by derived manufacturing-analysis geometry.
     */
    using AnalysisIndex = std::uint32_t;

    /**
     * @brief Vertex in the triangulated manufacturing-analysis mesh.
     */
    struct AnalysisVertex {
        /**
         * @brief Vertex position in object space.
         */
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief Source face normal associated with this derived vertex.
         */
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
    };

    /**
     * @brief Indexed triangle in the manufacturing-analysis mesh.
     */
    struct AnalysisTriangle {
        /**
         * @brief First analysis vertex index.
         */
        AnalysisIndex a = 0;

        /**
         * @brief Second analysis vertex index.
         */
        AnalysisIndex b = 0;

        /**
         * @brief Third analysis vertex index.
         */
        AnalysisIndex c = 0;
    };

    /**
     * @brief Derived triangulated representation used by manufacturing
     * analyzers.
     *
     * AnalysisMesh is reconstructed from the editable LEM and must never be
     * treated as authoritative editable geometry. Its topology is optimized
     * for analysis rather than editing.
     */
    class AnalysisMesh {
    public:
        /**
         * @brief Checks whether no analysis geometry is stored.
         *
         * @return True when both vertex and triangle collections are empty.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return vertices_.empty() && triangles_.empty();
        }

        /**
         * @brief Returns the number of derived vertices.
         *
         * @return Analysis vertex count.
         */
        [[nodiscard]] std::size_t vertex_count() const noexcept
        {
            return vertices_.size();
        }

        /**
         * @brief Returns the number of derived triangles.
         *
         * @return Analysis triangle count.
         */
        [[nodiscard]] std::size_t triangle_count() const noexcept
        {
            return triangles_.size();
        }

        /**
         * @brief Returns a derived vertex by index.
         *
         * @param index Vertex index.
         * @return Read-only analysis vertex.
         */
        [[nodiscard]] const AnalysisVertex& vertex(
            AnalysisIndex index) const
        {
            return vertices_[index];
        }

        /**
         * @brief Returns a derived triangle by index.
         *
         * @param index Triangle index.
         * @return Read-only analysis triangle.
         */
        [[nodiscard]] const AnalysisTriangle& triangle(
            AnalysisIndex index) const
        {
            return triangles_[index];
        }

        /**
         * @brief Returns all analysis vertices.
         *
         * @return Read-only vertex collection.
         */
        [[nodiscard]] const std::vector<AnalysisVertex>&
            vertices() const noexcept
        {
            return vertices_;
        }

        /**
         * @brief Returns all analysis triangles.
         *
         * @return Read-only triangle collection.
         */
        [[nodiscard]] const std::vector<AnalysisTriangle>&
            triangles() const noexcept
        {
            return triangles_;
        }

        /**
         * @brief Returns mappings from derived primitives to editable LEM
         * elements.
         *
         * @return Read-only handle mapping.
         */
        [[nodiscard]] const MeshHandleMapping& mapping() const noexcept
        {
            return mapping_;
        }

        /**
         * @brief Returns object-space bounds of the derived analysis geometry.
         *
         * @return Analysis mesh bounds.
         */
        [[nodiscard]] const math::Bounds& bounds() const noexcept
        {
            return bounds_;
        }

        /**
         * @brief Checks whether the analysis mesh has valid spatial bounds.
         *
         * @return True when at least one position contributed to the bounds.
         */
        [[nodiscard]] bool has_bounds() const
        {
            return bounds_.is_valid();
        }

        /**
         * @brief Removes all derived geometry and source mappings.
         */
        void clear()
        {
            vertices_.clear();
            triangles_.clear();
            mapping_.clear();
            bounds_.reset();
        }

    private:
        friend class AnalysisMeshBuilder;

        /**
         * @brief Appends a derived analysis vertex.
         *
         * @param vertex Vertex to append.
         * @return Index assigned to the vertex.
         */
        [[nodiscard]] AnalysisIndex add_vertex(
            const AnalysisVertex& vertex)
        {
            const AnalysisIndex index =
                static_cast<AnalysisIndex>(vertices_.size());

            vertices_.push_back(vertex);
            bounds_.expand(vertex.position);

            return index;
        }

        /**
         * @brief Appends a derived triangle and its source-face mapping.
         *
         * @param triangle Triangle to append.
         * @param sourceFace Editable LEM face represented by the triangle.
         */
        void add_triangle(
            const AnalysisTriangle& triangle,
            geometry::FaceHandle sourceFace)
        {
            triangles_.push_back(triangle);
            mapping_.add_triangle_face(sourceFace);
        }

        std::vector<AnalysisVertex> vertices_{};
        std::vector<AnalysisTriangle> triangles_{};

        MeshHandleMapping mapping_{};

        math::Bounds bounds_ = math::Bounds::empty();
    };

}