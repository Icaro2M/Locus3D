/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/debug/DebugDraw.h"

#include "graphics/common/GraphicsError.h"

#include <glm/geometric.hpp>

#include <utility>

namespace locus::graphics
{
    namespace
    {
        constexpr const char* DebugDrawShaderName = "debug/draw";
    }

    DebugDraw::~DebugDraw()
    {
        destroy();
    }

    DebugDraw::DebugDraw(DebugDraw&& other) noexcept
    {
        *this = std::move(other);
    }

    DebugDraw& DebugDraw::operator=(DebugDraw&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        config_ = std::move(other.config_);
        shader_ = other.shader_;
        vertices_ = std::move(other.vertices_);
        mesh_ = std::move(other.mesh_);
        object_ = std::move(other.object_);

        if (mesh_.is_valid())
        {
            // The moved render object may still point at the old instance's mesh.
            object_.mesh = &mesh_;
        }

        object_.shader = shader_;

        other.shader_ = nullptr;
        other.vertices_.clear();
        other.object_.mesh = nullptr;
        other.object_.shader = nullptr;

        return *this;
    }

    GraphicsResult<void> DebugDraw::create(
        const ShaderManager& shaderManager,
        const DebugDrawConfig& config
    )
    {
        destroy();

        const Shader* shader = shaderManager.find(DebugDrawShaderName);
        if (shader == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "DebugDraw requires shader: debug/draw."
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

    void DebugDraw::destroy()
    {
        mesh_.destroy();
        vertices_.clear();

        object_.mesh = nullptr;
        object_.shader = nullptr;
        shader_ = nullptr;

        config_ = {};
    }

    void DebugDraw::clear()
    {
        vertices_.clear();
        mesh_.destroy();
        object_.mesh = nullptr;
    }

    void DebugDraw::add_line(const glm::vec3& a, const glm::vec3& b)
    {
        add_line(a, b, config_.defaultColor);
    }

    void DebugDraw::add_line(const glm::vec3& a, const glm::vec3& b, const ColorRGBA& color)
    {
        vertices_.push_back(make_vertex(a, color));
        vertices_.push_back(make_vertex(b, color));
    }

    void DebugDraw::add_ray(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float length
    )
    {
        add_ray(origin, direction, length, config_.defaultColor);
    }

    void DebugDraw::add_ray(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float length,
        const ColorRGBA& color
    )
    {
        if (length <= 0.0f)
        {
            return;
        }

        const float directionLength = glm::length(direction);
        if (directionLength <= 0.0f)
        {
            return;
        }

        // Normalize only after rejecting zero-length directions to avoid NaNs.
        const glm::vec3 end = origin + glm::normalize(direction) * length;
        add_line(origin, end, color);
    }

    void DebugDraw::add_box(
        const glm::vec3& minPoint,
        const glm::vec3& maxPoint
    )
    {
        add_box(minPoint, maxPoint, config_.defaultColor);
    }

    void DebugDraw::add_box(
        const glm::vec3& minPoint,
        const glm::vec3& maxPoint,
        const ColorRGBA& color
    )
    {
        const glm::vec3 p000{ minPoint.x, minPoint.y, minPoint.z };
        const glm::vec3 p001{ minPoint.x, minPoint.y, maxPoint.z };
        const glm::vec3 p010{ minPoint.x, maxPoint.y, minPoint.z };
        const glm::vec3 p011{ minPoint.x, maxPoint.y, maxPoint.z };
        const glm::vec3 p100{ maxPoint.x, minPoint.y, minPoint.z };
        const glm::vec3 p101{ maxPoint.x, minPoint.y, maxPoint.z };
        const glm::vec3 p110{ maxPoint.x, maxPoint.y, minPoint.z };
        const glm::vec3 p111{ maxPoint.x, maxPoint.y, maxPoint.z };

        add_line(p000, p100, color);
        add_line(p100, p101, color);
        add_line(p101, p001, color);
        add_line(p001, p000, color);

        add_line(p010, p110, color);
        add_line(p110, p111, color);
        add_line(p111, p011, color);
        add_line(p011, p010, color);

        add_line(p000, p010, color);
        add_line(p100, p110, color);
        add_line(p101, p111, color);
        add_line(p001, p011, color);
    }

    GraphicsResult<void> DebugDraw::upload(const MeshUploader& uploader)
    {
        mesh_.destroy();
        object_.mesh = nullptr;

        if (shader_ == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload DebugDraw because it was not created."
            );
        }

        if (vertices_.empty())
        {
            return {};
        }

        MeshUploadData data;
        data.topology = PrimitiveTopology::Lines;
        data.usage = BufferUsage::Dynamic;
        data.vertices = vertices_;

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

    void DebugDraw::submit(RenderScene& scene) const
    {
        if (!is_valid())
        {
            return;
        }

        scene.add_object(object_);
    }

    bool DebugDraw::is_valid() const
    {
        return object_.is_drawable();
    }

    bool DebugDraw::has_geometry() const
    {
        return !vertices_.empty();
    }

    std::size_t DebugDraw::line_count() const
    {
        return vertices_.size() / 2;
    }

    const RenderObject& DebugDraw::render_object() const
    {
        return object_;
    }

    MeshVertex DebugDraw::make_vertex(const glm::vec3& position, const ColorRGBA& color)
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
