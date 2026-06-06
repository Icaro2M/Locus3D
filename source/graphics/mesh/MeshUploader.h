/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploadData.h"

namespace locus::graphics
{

    /**
     * @brief Factory object that converts CPU mesh data into a GpuMesh.
     */
    class MeshUploader
    {
    public:
        /**
         * @brief Creates a stateless mesh uploader.
         */
        MeshUploader() = default;

        /**
         * @brief Destroys the uploader.
         */
        ~MeshUploader() = default;

        MeshUploader(const MeshUploader&) = delete;
        MeshUploader& operator=(const MeshUploader&) = delete;

        MeshUploader(MeshUploader&&) = delete;
        MeshUploader& operator=(MeshUploader&&) = delete;

        /**
         * @brief Uploads CPU mesh data into GPU resources.
         *
         * @param uploadData CPU-side mesh payload.
         * @return Created mesh or upload error.
         */
        [[nodiscard]] GraphicsResult<GpuMesh> upload(const MeshUploadData& uploadData) const;
    };

}
