/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/AxisRenderer.h"

#include <utility>

namespace locus::graphics
{
    AxisRenderer::~AxisRenderer()
    {
        destroy();
    }

    AxisRenderer::AxisRenderer(AxisRenderer&& other) noexcept
        : config_(other.config_),
        shader_(std::move(other.shader_)),
        mesh_(std::move(other.mesh_)),
        object_(std::move(other.object_))
    {
        // RenderObject stores raw resource pointers, so moves must rebind them.
        object_.shader = &shader_;
        object_.mesh = &mesh_;
        other.object_.shader = nullptr;
        other.object_.mesh = nullptr;
    }

    AxisRenderer& AxisRenderer::operator=(AxisRenderer&& other) noexcept
    {
        if (this != &other)
        {
            destroy();

            config_ = other.config_;
            shader_ = std::move(other.shader_);
            mesh_ = std::move(other.mesh_);
            object_ = std::move(other.object_);

            // RenderObject stores raw resource pointers, so moves must rebind them.
            object_.shader = &shader_;
            object_.mesh = &mesh_;
            other.object_.shader = nullptr;
            other.object_.mesh = nullptr;
        }

        return *this;
    }

    GraphicsResult<void> AxisRenderer::create(
        const MeshUploader& uploader,
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

        config_ = config;

        auto shaderResult = shader_.create_from_source(
            vertex_shader_source(),
            fragment_shader_source()
        );

        if (!shaderResult)
        {
            return shaderResult.error();
        }

        MeshUploadData meshData = build_mesh_data(config_);
        auto meshResult = uploader.upload(meshData);

        if (!meshResult)
        {
            shader_.destroy();
            return meshResult.error();
        }

        mesh_ = meshResult.move_value();

        object_.id = 1002;
        object_.name = "ViewportAxes";
        object_.mesh = &mesh_;
        object_.shader = &shader_;
        object_.layer = RenderLayer::Overlay;

        return {};
    }

    void AxisRenderer::destroy()
    {
        mesh_.destroy();
        shader_.destroy();

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
            { 0.0f, 0.0f, -config.extent },
            { 0.0f, 0.0f, config.extent },
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

    const char* AxisRenderer::vertex_shader_source()
    {
        return R"(
#version 450 core

layout (location = 0) in vec3 a_Position;
layout (location = 2) in vec4 a_Color;

uniform mat4 u_MVP;

out vec4 v_Color;

void main()
{
    v_Color = a_Color;
    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";
    }

    const char* AxisRenderer::fragment_shader_source()
    {
        return R"(
#version 450 core

in vec4 v_Color;

out vec4 FragColor;

void main()
{
    FragColor = v_Color;
}
)";
    }
}
