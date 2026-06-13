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
    struct MeasurementDrawItem
    {
        glm::vec3 start{ 0.0f, 0.0f, 0.0f };
        glm::vec3 end{ 1.0f, 0.0f, 0.0f };
        ColorRGBA color{ 1.0f, 0.85f, 0.15f, 1.0f };
        bool drawTicks = true;
    };

    struct MeasurementRendererConfig
    {
        RenderObject::Id objectId = 1005;
        std::string objectName = "Measurements";
        RenderLayer layer = RenderLayer::Overlay;
        ColorRGBA defaultColor{ 1.0f, 0.85f, 0.15f, 1.0f };
        float tickLength = 0.18f;
    };

    class MeasurementRenderer
    {
    public:
        MeasurementRenderer() = default;
        ~MeasurementRenderer();

        MeasurementRenderer(const MeasurementRenderer&) = delete;
        MeasurementRenderer& operator=(const MeasurementRenderer&) = delete;

        MeasurementRenderer(MeasurementRenderer&& other) noexcept;
        MeasurementRenderer& operator=(MeasurementRenderer&& other) noexcept;

        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const MeasurementRendererConfig& config = {}
        );

        void destroy();
        void clear();

        void add_measurement(const glm::vec3& start, const glm::vec3& end);
        void add_measurement(
            const glm::vec3& start,
            const glm::vec3& end,
            const ColorRGBA& color
        );
        void add_measurement(
            const glm::vec3& start,
            const glm::vec3& end,
            const ColorRGBA& color,
            bool drawTicks
        );
        void add_measurement(const MeasurementDrawItem& item);

        [[nodiscard]] GraphicsResult<void> upload(const MeshUploader& uploader);

        void submit(RenderScene& scene) const;

        [[nodiscard]] bool is_valid() const;
        [[nodiscard]] bool has_measurements() const;
        [[nodiscard]] std::size_t measurement_count() const;

        [[nodiscard]] const RenderObject& render_object() const;

    private:
        static MeshVertex make_vertex(const glm::vec3& position, const ColorRGBA& color);
        static glm::vec3 build_tick_axis(const glm::vec3& direction);

        void append_line(
            std::vector<MeshVertex>& vertices,
            const glm::vec3& start,
            const glm::vec3& end,
            const ColorRGBA& color
        ) const;

        void append_measurement(
            std::vector<MeshVertex>& vertices,
            const MeasurementDrawItem& item
        ) const;

    private:
        MeasurementRendererConfig config_{};
        const Shader* shader_ = nullptr;
        std::vector<MeasurementDrawItem> measurements_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}