/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/mesh/MeshUploader.h"

namespace locus::graphics
{

    GraphicsResult<GpuMesh> MeshUploader::upload(const MeshUploadData& uploadData) const
    {
        GpuMesh mesh;

        auto result = mesh.create(uploadData);

        if (!result)
        {
            return result.error();
        }

        return mesh;
    }

}
