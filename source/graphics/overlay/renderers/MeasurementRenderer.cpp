#include "graphics/overlay/renderers/MeasurementRenderer.h"

#include "graphics/common/GraphicsError.h"

#include <glm/geometric.hpp>

#include <utility>

namespace locus::graphics
{
    namespace
    {
        constexpr const char* MeasurementRendererShaderName = "debug/draw";
        constexpr float MinMeasurementLength = 0.00001f;
    }

    MeasurementRenderer::~MeasurementRenderer()
    {
        destroy();
    }

    MeasurementRenderer::MeasurementRenderer(MeasurementRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    MeasurementRenderer& MeasurementRenderer::operator=(MeasurementRenderer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        config_ = std::move(other.config_);
        shader_ = other.shader_;
        measurements_ = std::move(other.measurements_);
        mesh_ = std::move(other.mesh_);
        object_ = std::move(other.object_);

        if (mesh_.is_valid())
        {
            object_.mesh = &mesh_;
        }

        object_.shader = shader_;

        other.shader_ = nullptr;
        other.measurements_.clear();
        other.object_.mesh = nullptr;
        other.object_.shader = nullptr;

        return *this;
    }

    GraphicsResult<void> MeasurementRenderer::create(
        const ShaderManager& shaderManager,
        const MeasurementRendererConfig& config
    )
    {
        destroy();

        const Shader* shader = shaderManager.find(MeasurementRendererShaderName);

        if (shader == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "MeasurementRenderer requires shader: debug/draw."
            );
        }

        config_ = config;
        shader_ = shader;

        object_.id = config_.objectId;
        object_.name = config_.objectName;
        object_.mesh = nullptr;
        object_.shader = shader_;
        object_.layer = config_.layer;
        object_.visibility.selectable = false;

        return {};
    }

    void MeasurementRenderer::destroy()
    {
        mesh_.destroy();
        measurements_.clear();

        object_.mesh = nullptr;
        object_.shader = nullptr;

        shader_ = nullptr;
        config_ = {};
    }

    void MeasurementRenderer::clear()
    {
        measurements_.clear();
        mesh_.destroy();
        object_.mesh = nullptr;
    }

    void MeasurementRenderer::add_measurement(const glm::vec3& start, const glm::vec3& end)
    {
        add_measurement(start, end, config_.defaultColor, true);
    }

    void MeasurementRenderer::add_measurement(
        const glm::vec3& start,
        const glm::vec3& end,
        const ColorRGBA& color
    )
    {
        add_measurement(start, end, color, true);
    }

    void MeasurementRenderer::add_measurement(
        const glm::vec3& start,
        const glm::vec3& end,
        const ColorRGBA& color,
        bool drawTicks
    )
    {
        const glm::vec3 delta = end - start;

        if (glm::length(delta) <= MinMeasurementLength)
        {
            return;
        }

        MeasurementDrawItem item;
        item.start = start;
        item.end = end;
        item.color = color;
        item.drawTicks = drawTicks;

        measurements_.push_back(item);
    }

    void MeasurementRenderer::add_measurement(const MeasurementDrawItem& item)
    {
        add_measurement(item.start, item.end, item.color, item.drawTicks);
    }

    GraphicsResult<void> MeasurementRenderer::upload(const MeshUploader& uploader)
    {
        mesh_.destroy();
        object_.mesh = nullptr;

        if (shader_ == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload MeasurementRenderer because it was not created."
            );
        }

        if (measurements_.empty())
        {
            return {};
        }

        std::vector<MeshVertex> vertices;
        vertices.reserve(measurements_.size() * 6);

        for (const MeasurementDrawItem& item : measurements_)
        {
            append_measurement(vertices, item);
        }

        if (vertices.empty())
        {
            return {};
        }

        MeshUploadData data;
        data.vertices = std::move(vertices);
        data.topology = PrimitiveTopology::Lines;
        data.usage = BufferUsage::Dynamic;

        auto meshResult = uploader.upload(data);

        if (!meshResult)
        {
            return meshResult.error();
        }

        mesh_ = meshResult.move_value();
        object_.mesh = &mesh_;
        object_.shader = shader_;

        return {};
    }

    void MeasurementRenderer::submit(RenderScene& scene) const
    {
        if (!is_valid())
        {
            return;
        }

        scene.add_object(object_);
    }

    bool MeasurementRenderer::is_valid() const
    {
        return object_.is_drawable();
    }

    bool MeasurementRenderer::has_measurements() const
    {
        return !measurements_.empty();
    }

    std::size_t MeasurementRenderer::measurement_count() const
    {
        return measurements_.size();
    }

    const RenderObject& MeasurementRenderer::render_object() const
    {
        return object_;
    }

    MeshVertex MeasurementRenderer::make_vertex(
        const glm::vec3& position,
        const ColorRGBA& color
    )
    {
        MeshVertex vertex;

        vertex.position[0] = position.x;
        vertex.position[1] = position.y;
        vertex.position[2] = position.z;

        vertex.normal[0] = 0.0f;
        vertex.normal[1] = 1.0f;
        vertex.normal[2] = 0.0f;

        vertex.color[0] = color.r;
        vertex.color[1] = color.g;
        vertex.color[2] = color.b;
        vertex.color[3] = color.a;

        return vertex;
    }

    glm::vec3 MeasurementRenderer::build_tick_axis(const glm::vec3& direction)
    {
        glm::vec3 reference{ 0.0f, 1.0f, 0.0f };

        if (glm::abs(glm::dot(direction, reference)) > 0.9f)
        {
            reference = { 1.0f, 0.0f, 0.0f };
        }

        const glm::vec3 axis = glm::cross(direction, reference);
        const float axisLength = glm::length(axis);

        if (axisLength <= MinMeasurementLength)
        {
            return { 1.0f, 0.0f, 0.0f };
        }

        return axis / axisLength;
    }

    void MeasurementRenderer::append_line(
        std::vector<MeshVertex>& vertices,
        const glm::vec3& start,
        const glm::vec3& end,
        const ColorRGBA& color
    ) const
    {
        vertices.push_back(make_vertex(start, color));
        vertices.push_back(make_vertex(end, color));
    }

    void MeasurementRenderer::append_measurement(
        std::vector<MeshVertex>& vertices,
        const MeasurementDrawItem& item
    ) const
    {
        const glm::vec3 delta = item.end - item.start;
        const float length = glm::length(delta);

        if (length <= MinMeasurementLength)
        {
            return;
        }

        const glm::vec3 direction = delta / length;

        append_line(vertices, item.start, item.end, item.color);

        if (!item.drawTicks || config_.tickLength <= 0.0f)
        {
            return;
        }

        const glm::vec3 tickAxis = build_tick_axis(direction);
        const glm::vec3 halfTick = tickAxis * (config_.tickLength * 0.5f);

        append_line(vertices, item.start - halfTick, item.start + halfTick, item.color);
        append_line(vertices, item.end - halfTick, item.end + halfTick, item.color);
    }
}