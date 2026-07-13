/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/primitives/PrimitiveVertex.h"

#include <cstddef>
#include <vector>

namespace locus::graphics {

    /**
     * @brief CPU-side geometry for a single primitive topology.
     *
     * PrimitiveMesh stores vertices and optional indices used to describe one
     * drawable group of points, lines, or triangles before conversion to a
     * MeshUploadData payload.
     *
     * Each instance uses exactly one primitive topology. The type does not own GPU
     * resources and does not contain render state, material, scene, or
     * editor-specific information.
     */
    struct PrimitiveMesh {
        /**
         * @brief Vertices that compose the primitive geometry.
         */
        std::vector<PrimitiveVertex> vertices;

        /**
         * @brief Optional 32-bit indices into the vertex list.
         *
         * When this list is empty, vertices are interpreted in sequential order.
         */
        std::vector<u32> indices;

        /**
         * @brief Primitive assembly mode used by the mesh.
         */
        PrimitiveTopology topology = PrimitiveTopology::Triangles;

        /**
         * @brief Checks whether indexed drawing should be used.
         *
         * @return True when at least one index is present.
         */
        [[nodiscard]] bool has_indices() const {
            return !indices.empty();
        }

        /**
         * @brief Checks whether the mesh has no vertices.
         *
         * @return True when the vertex list is empty.
         */
        [[nodiscard]] bool is_empty() const {
            return vertices.empty();
        }

        /**
         * @brief Returns the number of elements consumed by primitive assembly.
         *
         * The index count is returned for indexed meshes. Otherwise, the vertex
         * count is returned.
         *
         * @return Number of indices or sequential vertices used for drawing.
         */
        [[nodiscard]] std::size_t element_count() const {
            return has_indices() ? indices.size() : vertices.size();
        }

        /**
         * @brief Checks whether the mesh contains structurally valid geometry.
         *
         * Validation checks that the mesh has vertices, every index references an
         * existing vertex, and the element count is compatible with the selected
         * primitive topology.
         *
         * @return True when the mesh can be converted into an upload payload.
         */
        [[nodiscard]] bool is_valid() const {
            if (vertices.empty()) {
                return false;
            }

            for (const u32 index : indices) {
                if (index >= vertices.size()) {
                    return false;
                }
            }

            const std::size_t count = element_count();

            switch (topology) {
            case PrimitiveTopology::Points:
                return count >= 1;

            case PrimitiveTopology::Lines:
                return count >= 2 && count % 2 == 0;

            case PrimitiveTopology::LineStrip:
                return count >= 2;

            case PrimitiveTopology::Triangles:
                return count >= 3 && count % 3 == 0;

            case PrimitiveTopology::TriangleStrip:
                return count >= 3;
            }

            return false;
        }
    };

} // namespace locus::graphics