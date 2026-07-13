/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/mesh/MeshUploadData.h"
#include "graphics/primitives/PrimitiveMesh.h"

namespace locus::graphics {

    /**
     * @brief Converts primitive geometry into mesh upload payloads.
     *
     * PrimitiveMeshConverter bridges the convenient CPU-side representation used
     * by PrimitiveBuilder and the vertex layout expected by MeshUploader.
     *
     * The converter does not create GPU resources, perform OpenGL calls, or modify
     * the source primitive mesh.
     */
    class PrimitiveMeshConverter {
    public:
        /**
         * @brief Converts a primitive mesh into a mesh upload payload.
         *
         * Vertex positions, normals, colors, indices, and primitive topology are
         * preserved during conversion. The requested buffer usage is assigned to
         * the resulting upload payload.
         *
         * @param mesh Source primitive mesh.
         * @param usage Expected GPU buffer update frequency.
         * @return Upload payload compatible with MeshUploader.
         */
        [[nodiscard]] static MeshUploadData to_upload_data(
            const PrimitiveMesh& mesh,
            BufferUsage usage = BufferUsage::Dynamic
        );
    };

} // namespace locus::graphics