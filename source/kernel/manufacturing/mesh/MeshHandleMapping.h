/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "kernel/geometry/mesh/LEMHandles.h"

#include <cstddef>
#include <vector>

namespace locus::kernel::manufacturing {

    /**
     * @brief Maps derived analysis primitives back to authoritative LEM
     * elements.
     *
     * Analysis triangle indices are transient and may change whenever the
     * derived AnalysisMesh is rebuilt. The mapping allows analyzers to report
     * findings using stable source mesh handles instead.
     */
    class MeshHandleMapping {
    public:
        /**
         * @brief Removes all stored mappings.
         */
        void clear()
        {
            triangleFaces_.clear();
        }

        /**
         * @brief Returns the number of mapped analysis triangles.
         *
         * @return Triangle-to-face mapping count.
         */
        [[nodiscard]] std::size_t triangle_count() const noexcept
        {
            return triangleFaces_.size();
        }

        /**
         * @brief Checks whether a mapping exists for an analysis triangle.
         *
         * @param triangleIndex Analysis triangle index.
         * @return True when the index has a source-face mapping.
         */
        [[nodiscard]] bool has_triangle(
            std::size_t triangleIndex) const noexcept
        {
            return triangleIndex < triangleFaces_.size();
        }

        /**
         * @brief Returns the source LEM face represented by an analysis
         * triangle.
         *
         * @param triangleIndex Analysis triangle index.
         * @return Source face, or an invalid handle when the index is outside
         * the mapping.
         */
        [[nodiscard]] geometry::FaceHandle face_for_triangle(
            std::size_t triangleIndex) const noexcept
        {
            if (!has_triangle(triangleIndex)) {
                return {};
            }

            return triangleFaces_[triangleIndex];
        }

        /**
         * @brief Returns all triangle-to-face mappings.
         *
         * @return Read-only source-face collection indexed by analysis
         * triangle.
         */
        [[nodiscard]] const std::vector<geometry::FaceHandle>&
            triangle_faces() const noexcept
        {
            return triangleFaces_;
        }

    private:
        friend class AnalysisMesh;

        /**
         * @brief Appends the source face of a newly created analysis triangle.
         *
         * @param face Source editable-mesh face.
         */
        void add_triangle_face(geometry::FaceHandle face)
        {
            triangleFaces_.push_back(face);
        }

        std::vector<geometry::FaceHandle> triangleFaces_{};
    };

}