#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace locus::graphics
{
    struct BoundingBoxDrawItem
    {
        glm::vec3 minPoint{ 0.0f, 0.0f, 0.0f };
        glm::vec3 maxPoint{ 0.0f, 0.0f, 0.0f };
        ColorRGBA color{ 0.2f, 0.85f, 1.0f, 1.0f };
    };

    struct BoundingBoxRendererConfig
    {
        RenderObject::Id objectId = 1003;
        std::string objectName = "BoundingBoxes";
        RenderLayer layer = RenderLayer::Overlay;
        ColorRGBA defaultColor{ 0.2f, 0.85f, 1.0f, 1.0f };
    };

    class BoundingBoxRenderer
    {
    public:
        BoundingBoxRenderer() = default;
        ~BoundingBoxRenderer();

        BoundingBoxRenderer(const BoundingBoxRenderer&) = delete;
        BoundingBoxRenderer& operator=(const BoundingBoxRenderer&) = delete;

        BoundingBoxRenderer(BoundingBoxRenderer&& other) noexcept;
        BoundingBoxRenderer& operator=(BoundingBoxRenderer&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const BoundingBoxRendererConfig& config = {}
        );

        void destroy();
        void clear();

        void add_box(const glm::vec3& minPoint, const glm::vec3& maxPoint);
        void add_box(const glm::vec3& minPoint, const glm::vec3& maxPoint, const ColorRGBA& color);
        void add_box(const BoundingBoxDrawItem& item);

        [[nodiscard]] GraphicsResult<void> upload(const MeshUploader& uploader);
        void submit(RenderScene& scene) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] bool has_boxes() const;
        [[nodiscard]] std::size_t box_count() const;
        [[nodiscard]] const RenderObject& render_object() const;

    private:
        static MeshVertex make_vertex(const glm::vec3& position, const ColorRGBA& color);
        static void add_line(
            std::vector<MeshVertex>& vertices,
            const glm::vec3& a,
            const glm::vec3& b,
            const ColorRGBA& color
        );

        static void append_box_vertices(
            std::vector<MeshVertex>& vertices,
            const glm::vec3& minPoint,
            const glm::vec3& maxPoint,
            const ColorRGBA& color
        );

    private:
        BoundingBoxRendererConfig config_{};
        const Shader* shader_ = nullptr;
        std::vector<BoundingBoxDrawItem> boxes_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}