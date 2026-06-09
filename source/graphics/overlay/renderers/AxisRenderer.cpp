/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/AxisRenderer.h"

#include "graphics/common/GraphicsError.h"

#include <utility>

namespace locus::graphics
{
    namespace
    {
        constexpr const char* AxisShaderName = "viewport/axis";
    }

    AxisRenderer::~AxisRenderer()
    {
        destroy();
    }

    AxisRenderer::AxisRenderer(AxisRenderer&& other) noexcept
        : config_(other.config_),
        mesh_(std::move(other.mesh_)),
        object_(std::move(other.object_))
    {
        // RenderObject stores a raw mesh pointer, so moves must rebind it.
        object_.mesh = &mesh_;
        other.object_.mesh = nullptr;
        other.object_.shader = nullptr;
    }

    AxisRenderer& AxisRenderer::operator=(AxisRenderer&& other) noexcept
    {
        if (this != &other)
        {
            destroy();

            config_ = other.config_;
            mesh_ = std::move(other.mesh_);
            object_ = std::move(other.object_);

            // RenderObject stores a raw mesh pointer, so moves must rebind it.
            object_.mesh = &mesh_;
            other.object_.mesh = nullptr;
            other.object_.shader = nullptr;
        }

        return *this;
    }

    GraphicsResult<void> AxisRenderer::create(
        const MeshUploader& uploader,
        const ShaderManager& shaderManager,
        const AxisRendererConfig& config)
    {
        destroy();

        if (config.extent <= 0.0f || config.verticalExtent <= 0.0f)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "AxisRenderer received invalid configuration."
            );
        }

        const Shader* shader = shaderManager.find(AxisShaderName);

        if (shader == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "AxisRenderer requires shader: viewport/axis."
            );
        }

        config_ = config;

        MeshUploadData meshData = build_mesh_data(config_);
        auto meshResult = uploader.upload(meshData);

        if (!meshResult)
        {
            return meshResult.error();
        }

        mesh_ = meshResult.move_value();

        object_.id = 1002;
        object_.name = "ViewportAxes";
        object_.mesh = &mesh_;
        object_.shader = shader;
        object_.layer = RenderLayer::Overlay;

        return {};
    }

    void AxisRenderer::destroy()
    {
        mesh_.destroy();

        object_.mesh = nullptr;
        object_.shader = nullptr;
    }

    bool AxisRenderer::is_valid() const
    {
        return object_.is_drawable();
    }

    const RenderObject& AxisRenderer::render_object() const
    {
        return object_;
    }

    MeshUploadData AxisRenderer::build_mesh_data(const AxisRendererConfig& config)
    {
        MeshUploadData data;
        data.topology = PrimitiveTopology::Lines;
        data.usage = BufferUsage::Static;

        // Keep horizontal axes slightly above the grid to reduce depth fighting.
        const float y = config.planeOffset;

        add_line(
            data,
            { -config.extent, y, 0.0f },
            { config.extent, y, 0.0f },
            config.xColor
        );

        add_line(
            data,
            { 0.0f, y, -config.extent },
            { 0.0f, y, config.extent },
            config.zColor
        );

        add_line(
            data,
            { 0.0f, -config.verticalExtent, 0.0f },
            { 0.0f, config.verticalExtent, 0.0f },
            config.yColor
        );

        return data;
    }

    void AxisRenderer::add_line(
        MeshUploadData& data,
        const glm::vec3& a,
        const glm::vec3& b,
        const ColorRGBA& color)
    {
        MeshVertex vertexA;
        vertexA.position[0] = a.x;
        vertexA.position[1] = a.y;
        vertexA.position[2] = a.z;
        vertexA.normal[0] = 0.0f;
        vertexA.normal[1] = 1.0f;
        vertexA.normal[2] = 0.0f;
        vertexA.color[0] = color.r;
        vertexA.color[1] = color.g;
        vertexA.color[2] = color.b;
        vertexA.color[3] = color.a;

        MeshVertex vertexB;
        vertexB.position[0] = b.x;
        vertexB.position[1] = b.y;
        vertexB.position[2] = b.z;
        vertexB.normal[0] = 0.0f;
        vertexB.normal[1] = 1.0f;
        vertexB.normal[2] = 0.0f;
        vertexB.color[0] = color.r;
        vertexB.color[1] = color.g;
        vertexB.color[2] = color.b;
        vertexB.color[3] = color.a;

        data.vertices.push_back(vertexA);
        data.vertices.push_back(vertexB);
    }
}