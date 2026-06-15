/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/NormalRenderer.h"

#include "graphics/common/GraphicsError.h"

#include <glm/geometric.hpp>

#include <utility>

namespace locus::graphics
{
    namespace
    {
        constexpr const char* NormalRendererShaderName = "debug/draw";
    }

    NormalRenderer::~NormalRenderer()
    {
        destroy();
    }

    NormalRenderer::NormalRenderer(NormalRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    NormalRenderer& NormalRenderer::operator=(NormalRenderer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        config_ = std::move(other.config_);
        shader_ = other.shader_;
        normals_ = std::move(other.normals_);
        mesh_ = std::move(other.mesh_);
        object_ = std::move(other.object_);

        if (mesh_.is_valid())
        {
            // Restore the self-reference after moving the owned GPU mesh.
            object_.mesh = &mesh_;
        }

        object_.shader = shader_;

        other.shader_ = nullptr;
        other.normals_.clear();
        other.object_.mesh = nullptr;
        other.object_.shader = nullptr;

        return *this;
    }

    GraphicsResult<void> NormalRenderer::create(
        const ShaderManager& shaderManager,
        const NormalRendererConfig& config
    )
    {
        destroy();

        const Shader* shader = shaderManager.find(NormalRendererShaderName);

        if (shader == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "NormalRenderer requires shader: debug/draw."
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

    void NormalRenderer::destroy()
    {
        mesh_.destroy();
        normals_.clear();

        object_.mesh = nullptr;
        object_.shader = nullptr;

        shader_ = nullptr;
        config_ = {};
    }

    void NormalRenderer::clear()
    {
        normals_.clear();
        mesh_.destroy();
        object_.mesh = nullptr;
    }

    void NormalRenderer::add_normal(const glm::vec3& origin, const glm::vec3& normal)
    {
        add_normal(origin, normal, config_.defaultLength, config_.defaultColor);
    }

    void NormalRenderer::add_normal(
        const glm::vec3& origin,
        const glm::vec3& normal,
        float length
    )
    {
        add_normal(origin, normal, length, config_.defaultColor);
    }

    void NormalRenderer::add_normal(
        const glm::vec3& origin,
        const glm::vec3& normal,
        float length,
        const ColorRGBA& color
    )
    {
        if (length <= 0.0f)
        {
            return;
        }

        const float normalLength = glm::length(normal);

        if (normalLength <= 0.0f)
        {
            return;
        }

        NormalDrawItem item;
        item.origin = origin;
        item.normal = normal;
        item.length = length;
        item.color = color;

        normals_.push_back(item);
    }

    void NormalRenderer::add_normal(const NormalDrawItem& item)
    {
        add_normal(item.origin, item.normal, item.length, item.color);
    }

    GraphicsResult<void> NormalRenderer::upload(const MeshUploader& uploader)
    {
        mesh_.destroy();
        object_.mesh = nullptr;

        if (shader_ == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload NormalRenderer because it was not created."
            );
        }

        if (normals_.empty())
        {
            return {};
        }

        std::vector<MeshVertex> vertices;
        vertices.reserve(normals_.size() * 2);

        for (const NormalDrawItem& item : normals_)
        {
            const float normalLength = glm::length(item.normal);

            if (normalLength <= 0.0f || item.length <= 0.0f)
            {
                continue;
            }

            const glm::vec3 direction = item.normal / normalLength;
            const glm::vec3 end = item.origin + direction * item.length;

            vertices.push_back(make_vertex(item.origin, item.color));
            vertices.push_back(make_vertex(end, item.color));
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

    void NormalRenderer::submit(RenderScene& scene) const
    {
        if (!is_valid())
        {
            return;
        }

        scene.add_object(object_);
    }

    bool NormalRenderer::is_valid() const
    {
        return object_.is_drawable();
    }

    bool NormalRenderer::has_normals() const
    {
        return !normals_.empty();
    }

    std::size_t NormalRenderer::normal_count() const
    {
        return normals_.size();
    }

    const RenderObject& NormalRenderer::render_object() const
    {
        return object_;
    }

    MeshVertex NormalRenderer::make_vertex(const glm::vec3& position, const ColorRGBA& color)
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
}
