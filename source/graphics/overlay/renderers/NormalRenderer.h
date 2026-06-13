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
    struct NormalDrawItem
    {
        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        float length = 1.0f;
        ColorRGBA color{ 0.35f, 0.75f, 1.0f, 1.0f };
    };

    struct NormalRendererConfig
    {
        RenderObject::Id objectId = 1004;
        std::string objectName = "Normals";
        RenderLayer layer = RenderLayer::Overlay;
        float defaultLength = 0.25f;
        ColorRGBA defaultColor{ 0.35f, 0.75f, 1.0f, 1.0f };
    };

    class NormalRenderer
    {
    public:
        NormalRenderer() = default;
        ~NormalRenderer();

        NormalRenderer(const NormalRenderer&) = delete;
        NormalRenderer& operator=(const NormalRenderer&) = delete;

        NormalRenderer(NormalRenderer&& other) noexcept;
        NormalRenderer& operator=(NormalRenderer&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const NormalRendererConfig& config = {}
        );

        void destroy();
        void clear();

        void add_normal(const glm::vec3& origin, const glm::vec3& normal);
        void add_normal(const glm::vec3& origin, const glm::vec3& normal, float length);
        void add_normal(
            const glm::vec3& origin,
            const glm::vec3& normal,
            float length,
            const ColorRGBA& color
        );
        void add_normal(const NormalDrawItem& item);

        [[nodiscard]] GraphicsResult<void> upload(const MeshUploader& uploader);

        void submit(RenderScene& scene) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] bool has_normals() const;
        [[nodiscard]] std::size_t normal_count() const;

        [[nodiscard]] const RenderObject& render_object() const;

    private:
        static MeshVertex make_vertex(const glm::vec3& position, const ColorRGBA& color);

    private:
        NormalRendererConfig config_{};
        const Shader* shader_ = nullptr;
        std::vector<NormalDrawItem> normals_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}