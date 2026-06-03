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