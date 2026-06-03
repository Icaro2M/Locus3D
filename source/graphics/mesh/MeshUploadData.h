#pragma once

#include "graphics/common/GraphicsTypes.h"

#include <vector>

namespace locus::graphics
{

    struct MeshVertex
    {
        f32 position[3]{ 0.0f, 0.0f, 0.0f };
        f32 normal[3]{ 0.0f, 0.0f, 1.0f };
        f32 color[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
    };

    struct MeshUploadData
    {
        std::vector<MeshVertex> vertices;
        std::vector<u32> indices;

        PrimitiveTopology topology = PrimitiveTopology::Triangles;
        BufferUsage usage = BufferUsage::Static;

        [[nodiscard]] bool has_indices() const
        {
            return !indices.empty();
        }

        [[nodiscard]] bool is_empty() const
        {
            return vertices.empty();
        }
    };

}