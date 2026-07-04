/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/render/RenderAdapterTypes.h"
#include "graphics/mesh/MeshUploadData.h"

namespace locus::kernel::geometry {

    class RenderMesh;

} // namespace locus::kernel::geometry

namespace locus::editor {

    /**
     * @brief Converts kernel render meshes into graphics upload payloads.
     *
     * This adapter is an integration layer only. It does not triangulate editable
     * meshes and does not create GPU resources. Geometry preparation remains in
     * kernel::geometry, while GPU upload remains in graphics.
     */
    class RenderMeshUploadAdapter {
    public:
        /**
         * @brief Converts RenderMesh triangle primitives to graphics upload data.
         *
         * @param mesh Source render mesh produced by the kernel.
         * @param options Conversion options.
         * @param result Optional conversion statistics output.
         * @return Triangle mesh upload data.
         */
        [[nodiscard]] static graphics::MeshUploadData build_triangle_upload_data(
            const kernel::geometry::RenderMesh& mesh,
            const RenderMeshUploadOptions& options = {},
            RenderMeshUploadResult* result = nullptr);

        /**
         * @brief Converts RenderMesh line primitives to graphics upload data.
         *
         * @param mesh Source render mesh produced by the kernel.
         * @param options Conversion options.
         * @param result Optional conversion statistics output.
         * @return Line mesh upload data.
         */
        [[nodiscard]] static graphics::MeshUploadData build_line_upload_data(
            const kernel::geometry::RenderMesh& mesh,
            const RenderMeshUploadOptions& options = {},
            RenderMeshUploadResult* result = nullptr);
    };

} // namespace locus::editor