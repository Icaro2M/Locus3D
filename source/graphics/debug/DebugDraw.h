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
#include <vector>

namespace locus::graphics
{
    struct DebugDrawConfig
    {
        RenderObject::Id objectId = 9001;
        std::string objectName = "DebugDraw";
        RenderLayer layer = RenderLayer::Debug;
        ColorRGBA defaultColor{ 1.0f, 0.85f, 0.15f, 1.0f };
    };

    class DebugDraw
    {
    public:
        DebugDraw() = default;
        ~DebugDraw();

        DebugDraw(const DebugDraw&) = delete;
        DebugDraw& operator=(const DebugDraw&) = delete;

        DebugDraw(DebugDraw&& other) noexcept;
        DebugDraw& operator=(DebugDraw&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const DebugDrawConfig& config = {}
        );

        void destroy();
        void clear();

        void add_line(const glm::vec3& a, const glm::vec3& b);
        void add_line(const glm::vec3& a, const glm::vec3& b, const ColorRGBA& color);

        void add_ray(
            const glm::vec3& origin,
            const glm::vec3& direction,
            float length
        );

        void add_ray(
            const glm::vec3& origin,
            const glm::vec3& direction,
            float length,
            const ColorRGBA& color
        );

        void add_box(
            const glm::vec3& minPoint,
            const glm::vec3& maxPoint
        );

        void add_box(
            const glm::vec3& minPoint,
            const glm::vec3& maxPoint,
            const ColorRGBA& color
        );

        [[nodiscard]] GraphicsResult<void> upload(const MeshUploader& uploader);
        void submit(RenderScene& scene) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] bool has_geometry() const;
        [[nodiscard]] std::size_t line_count() const;
        [[nodiscard]] const RenderObject& render_object() const;

    private:
        static MeshVertex make_vertex(const glm::vec3& position, const ColorRGBA& color);

    private:
        DebugDrawConfig config_{};
        const Shader* shader_ = nullptr;
        std::vector<MeshVertex> vertices_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}