/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/GridRenderer.h"

#include <cmath>
#include <utility>

namespace locus::graphics
{
    GridRenderer::~GridRenderer()
    {
        destroy();
    }

    GridRenderer::GridRenderer(GridRenderer&& other) noexcept
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

    GridRenderer& GridRenderer::operator=(GridRenderer&& other) noexcept
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

    GraphicsResult<void> GridRenderer::create(
        const MeshUploader& uploader,
        const GridRendererConfig& config)
    {
        destroy();

        if (config.halfExtent <= 0.0f ||
            config.minorSpacing <= 0.0f ||
            config.majorSpacing <= 0.0f ||
            config.fadeStart < 0.0f ||
            config.fadeEnd <= config.fadeStart)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "GridRenderer received invalid configuration."
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

        MeshUploadData meshData = build_mesh_data(config_.halfExtent);
        auto meshResult = uploader.upload(meshData);

        if (!meshResult)
        {
            shader_.destroy();
            return meshResult.error();
        }

        mesh_ = meshResult.move_value();

        object_.id = 1001;
        object_.name = "ViewportGrid";
        object_.mesh = &mesh_;
        object_.shader = &shader_;
        object_.layer = RenderLayer::Grid;
        object_.transform.position = { 0.0f, 0.0f, 0.0f };

        // Grid appearance is shader-driven; the mesh is only a large ground plane.
        shader_.bind();
        shader_.set_float("u_MinorSpacing", config_.minorSpacing);
        shader_.set_float("u_MajorSpacing", config_.majorSpacing);
        shader_.set_float("u_FadeStart", config_.fadeStart);
        shader_.set_float("u_FadeEnd", config_.fadeEnd);
        shader_.set_float("u_LineStrength", config_.lineStrength);
        shader_.set_float("u_MajorLineStrength", config_.majorLineStrength);
        shader_.set_float("u_AxisStrength", config_.axisStrength);
        shader_.set_vec4("u_MinorColor", config_.minorColor.r, config_.minorColor.g, config_.minorColor.b, config_.minorColor.a);
        shader_.set_vec4("u_MajorColor", config_.majorColor.r, config_.majorColor.g, config_.majorColor.b, config_.majorColor.a);
        shader_.set_vec4("u_XAxisColor", config_.xAxisColor.r, config_.xAxisColor.g, config_.xAxisColor.b, config_.xAxisColor.a);
        shader_.set_vec4("u_ZAxisColor", config_.zAxisColor.r, config_.zAxisColor.g, config_.zAxisColor.b, config_.zAxisColor.a);
        shader_.unbind();

        return {};
    }

    void GridRenderer::update(const Camera& camera)
    {
        if (!is_valid())
        {
            return;
        }

        const float snap = config_.minorSpacing;
        const glm::vec3& cameraPosition = camera.position();

        const float snappedX = std::floor(cameraPosition.x / snap) * snap;
        const float snappedZ = std::floor(cameraPosition.z / snap) * snap;

        // Snapping keeps the grid visually stable while following the camera.
        object_.transform.position = { snappedX, 0.0f, snappedZ };
    }

    void GridRenderer::destroy()
    {
        mesh_.destroy();
        shader_.destroy();

        object_.mesh = nullptr;
        object_.shader = nullptr;
    }

    bool GridRenderer::is_valid() const
    {
        return object_.is_drawable();
    }

    const RenderObject& GridRenderer::render_object() const
    {
        return object_;
    }

    MeshUploadData GridRenderer::build_mesh_data(float halfExtent)
    {
        MeshUploadData data;
        data.topology = PrimitiveTopology::Triangles;
        data.usage = BufferUsage::Static;

        // The fragment shader procedurally draws the grid on this plane.
        MeshVertex v0;
        v0.position[0] = -halfExtent;
        v0.position[1] = 0.0f;
        v0.position[2] = -halfExtent;
        v0.normal[0] = 0.0f;
        v0.normal[1] = 1.0f;
        v0.normal[2] = 0.0f;

        MeshVertex v1;
        v1.position[0] = halfExtent;
        v1.position[1] = 0.0f;
        v1.position[2] = -halfExtent;
        v1.normal[0] = 0.0f;
        v1.normal[1] = 1.0f;
        v1.normal[2] = 0.0f;

        MeshVertex v2;
        v2.position[0] = halfExtent;
        v2.position[1] = 0.0f;
        v2.position[2] = halfExtent;
        v2.normal[0] = 0.0f;
        v2.normal[1] = 1.0f;
        v2.normal[2] = 0.0f;

        MeshVertex v3;
        v3.position[0] = -halfExtent;
        v3.position[1] = 0.0f;
        v3.position[2] = halfExtent;
        v3.normal[0] = 0.0f;
        v3.normal[1] = 1.0f;
        v3.normal[2] = 0.0f;

        data.vertices = { v0, v1, v2, v3 };
        data.indices = { 0, 1, 2, 0, 2, 3 };

        return data;
    }

    const char* GridRenderer::vertex_shader_source()
    {
        return R"(
#version 450 core

layout (location = 0) in vec3 a_Position;

uniform mat4 u_Model;
uniform mat4 u_MVP;

out vec3 v_WorldPosition;
out vec3 v_LocalPosition;

void main()
{
    vec4 worldPosition = u_Model * vec4(a_Position, 1.0);

    v_WorldPosition = worldPosition.xyz;
    v_LocalPosition = a_Position;

    gl_Position = u_MVP * vec4(a_Position, 1.0);
}
)";
    }

    const char* GridRenderer::fragment_shader_source()
    {
        return R"(
#version 450 core

in vec3 v_WorldPosition;
in vec3 v_LocalPosition;

uniform float u_MinorSpacing;
uniform float u_MajorSpacing;
uniform float u_FadeStart;
uniform float u_FadeEnd;
uniform float u_LineStrength;
uniform float u_MajorLineStrength;
uniform float u_AxisStrength;

uniform vec4 u_MinorColor;
uniform vec4 u_MajorColor;
uniform vec4 u_XAxisColor;
uniform vec4 u_ZAxisColor;

out vec4 FragColor;

float grid_line(vec2 position, float spacing, float thickness)
{
    vec2 grid = abs(fract(position / spacing - 0.5) - 0.5) / fwidth(position / spacing);
    float line = min(grid.x, grid.y);
    return 1.0 - min(line / thickness, 1.0);
}

float axis_line(float value, float thickness)
{
    float distanceToAxis = abs(value) / fwidth(value);
    return 1.0 - min(distanceToAxis / thickness, 1.0);
}

void main()
{
    vec2 worldXZ = v_WorldPosition.xz;

    float minorLine = grid_line(worldXZ, u_MinorSpacing, 1.0);
    float majorLine = grid_line(worldXZ, u_MajorSpacing, 1.35);

    float xAxis = axis_line(v_WorldPosition.z, 1.8);
    float zAxis = axis_line(v_WorldPosition.x, 1.8);

    float distanceFromCameraPlane = length(v_LocalPosition.xz);
    float fade = 1.0 - smoothstep(u_FadeStart, u_FadeEnd, distanceFromCameraPlane);

    vec3 color = u_MinorColor.rgb * minorLine * u_LineStrength;
    color = max(color, u_MajorColor.rgb * majorLine * u_MajorLineStrength);
    color = max(color, u_XAxisColor.rgb * xAxis * u_AxisStrength);
    color = max(color, u_ZAxisColor.rgb * zAxis * u_AxisStrength);

    float visibility = max(max(minorLine * u_LineStrength, majorLine * u_MajorLineStrength), max(xAxis, zAxis) * u_AxisStrength);
    visibility *= fade;

    if (visibility <= 0.005)
    {
        discard;
    }

    FragColor = vec4(color * fade, 1.0);
}
)";
    }
}
