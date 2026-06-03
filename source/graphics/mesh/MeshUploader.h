#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploadData.h"

namespace locus::graphics
{

    class MeshUploader
    {
    public:
        MeshUploader() = default;
        ~MeshUploader() = default;

        MeshUploader(const MeshUploader&) = delete;
        MeshUploader& operator=(const MeshUploader&) = delete;

        MeshUploader(MeshUploader&&) = delete;
        MeshUploader& operator=(MeshUploader&&) = delete;

        [[nodiscard]] GraphicsResult<GpuMesh> upload(const MeshUploadData& uploadData) const;
    };

}