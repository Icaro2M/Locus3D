/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/GridRenderer.h"

#include "graphics/common/GraphicsError.h"

#include <cmath>
#include <utility>

namespace locus::graphics
{
    namespace
    {
        constexpr const char* GridShaderName = "viewport/grid";
    }

    GridRenderer::~GridRenderer()
    {
        destroy();
    }

    GridRenderer::GridRenderer(GridRenderer&& other) noexcept
        : config_(other.config_),
        mesh_(std::move(other.mesh_)),
        object_(std::move(other.object_))
    {
        // RenderObject stores a raw mesh pointer, so moves must rebind it.
        object_.mesh = &mesh_;
        other.object_.mesh = nullptr;
        other.object_.shader = nullptr;
    }

    GridRenderer& GridRenderer::operator=(GridRenderer&& other) noexcept
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

    GraphicsResult<void> GridRenderer::create(
        const MeshUploader& uploader,
        const ShaderManager& shaderManager,
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

        const Shader* shader = shaderManager.find(GridShaderName);

        if (shader == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "GridRenderer requires shader: viewport/grid."
            );
        }

        config_ = config;

        MeshUploadData meshData = build_mesh_data(config_.halfExtent);
        auto meshResult = uploader.upload(meshData);

        if (!meshResult)
        {
            return meshResult.error();
        }

        mesh_ = meshResult.move_value();

        object_.id = 1001;
        object_.name = "ViewportGrid";
        object_.mesh = &mesh_;
        object_.shader = shader;
        object_.layer = RenderLayer::Grid;
        object_.transform.position = { 0.0f, 0.0f, 0.0f };

        // Grid appearance is shader-driven; the mesh is only a large ground plane.
        shader->bind();
        shader->set_float("u_MinorSpacing", config_.minorSpacing);
        shader->set_float("u_MajorSpacing", config_.majorSpacing);
        shader->set_float("u_FadeStart", config_.fadeStart);
        shader->set_float("u_FadeEnd", config_.fadeEnd);
        shader->set_float("u_LineStrength", config_.lineStrength);
        shader->set_float("u_MajorLineStrength", config_.majorLineStrength);
        shader->set_vec4(
            "u_MinorColor",
            config_.minorColor.r,
            config_.minorColor.g,
            config_.minorColor.b,
            config_.minorColor.a
        );
        shader->set_vec4(
            "u_MajorColor",
            config_.majorColor.r,
            config_.majorColor.g,
            config_.majorColor.b,
            config_.majorColor.a
        );
        shader->unbind();

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
}