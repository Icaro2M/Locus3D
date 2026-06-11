/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/BoundingBoxRenderer.h"

#include "graphics/common/GraphicsError.h"

#include <algorithm>
#include <utility>

namespace locus::graphics
{
    namespace
    {
        constexpr const char* BoundingBoxShaderName = "viewport/bounding_box";
    }

    BoundingBoxRenderer::~BoundingBoxRenderer()
    {
        destroy();
    }

    BoundingBoxRenderer::BoundingBoxRenderer(BoundingBoxRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    BoundingBoxRenderer& BoundingBoxRenderer::operator=(BoundingBoxRenderer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        config_ = std::move(other.config_);
        shader_ = other.shader_;
        boxes_ = std::move(other.boxes_);
        mesh_ = std::move(other.mesh_);
        object_ = std::move(other.object_);

        if (mesh_.is_valid())
        {
            // Restore the self-reference after moving the owned GPU mesh.
            object_.mesh = &mesh_;
        }

        object_.shader = shader_;

        other.shader_ = nullptr;
        other.boxes_.clear();
        other.object_.mesh = nullptr;
        other.object_.shader = nullptr;

        return *this;
    }

    GraphicsResult<void> BoundingBoxRenderer::create(
        const ShaderManager& shaderManager,
        const BoundingBoxRendererConfig& config
    )
    {
        destroy();

        const Shader* shader = shaderManager.find(BoundingBoxShaderName);
        if (shader == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "BoundingBoxRenderer requires shader: viewport/bounding_box."
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

    void BoundingBoxRenderer::destroy()
    {
        mesh_.destroy();
        boxes_.clear();

        object_.mesh = nullptr;
        object_.shader = nullptr;
        shader_ = nullptr;

        config_ = {};
    }

    void BoundingBoxRenderer::clear()
    {
        boxes_.clear();
        mesh_.destroy();
        object_.mesh = nullptr;
    }

    void BoundingBoxRenderer::add_box(const glm::vec3& minPoint, const glm::vec3& maxPoint)
    {
        add_box(minPoint, maxPoint, config_.defaultColor);
    }

    void BoundingBoxRenderer::add_box(
        const glm::vec3& minPoint,
        const glm::vec3& maxPoint,
        const ColorRGBA& color
    )
    {
        BoundingBoxDrawItem item;
        // Normalize inverted bounds so callers can pass drag-selection corners directly.
        item.minPoint = glm::min(minPoint, maxPoint);
        item.maxPoint = glm::max(minPoint, maxPoint);
        item.color = color;

        boxes_.push_back(item);
    }

    void BoundingBoxRenderer::add_box(const BoundingBoxDrawItem& item)
    {
        add_box(item.minPoint, item.maxPoint, item.color);
    }

    GraphicsResult<void> BoundingBoxRenderer::upload(const MeshUploader& uploader)
    {
        mesh_.destroy();
        object_.mesh = nullptr;

        if (shader_ == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot upload BoundingBoxRenderer because it was not created."
            );
        }

        if (boxes_.empty())
        {
            return {};
        }

        std::vector<MeshVertex> vertices;
        vertices.reserve(boxes_.size() * 24);

        for (const BoundingBoxDrawItem& item : boxes_)
        {
            append_box_vertices(vertices, item.minPoint, item.maxPoint, item.color);
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

    void BoundingBoxRenderer::submit(RenderScene& scene) const
    {
        if (!is_valid())
        {
            return;
        }

        scene.add_object(object_);
    }

    bool BoundingBoxRenderer::is_valid() const
    {
        return object_.is_drawable();
    }

    bool BoundingBoxRenderer::has_boxes() const
    {
        return !boxes_.empty();
    }

    std::size_t BoundingBoxRenderer::box_count() const
    {
        return boxes_.size();
    }

    const RenderObject& BoundingBoxRenderer::render_object() const
    {
        return object_;
    }

    MeshVertex BoundingBoxRenderer::make_vertex(const glm::vec3& position, const ColorRGBA& color)
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

    void BoundingBoxRenderer::add_line(
        std::vector<MeshVertex>& vertices,
        const glm::vec3& a,
        const glm::vec3& b,
        const ColorRGBA& color
    )
    {
        vertices.push_back(make_vertex(a, color));
        vertices.push_back(make_vertex(b, color));
    }

    void BoundingBoxRenderer::append_box_vertices(
        std::vector<MeshVertex>& vertices,
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

        add_line(vertices, p000, p100, color);
        add_line(vertices, p100, p101, color);
        add_line(vertices, p101, p001, color);
        add_line(vertices, p001, p000, color);

        add_line(vertices, p010, p110, color);
        add_line(vertices, p110, p111, color);
        add_line(vertices, p111, p011, color);
        add_line(vertices, p011, p010, color);

        add_line(vertices, p000, p010, color);
        add_line(vertices, p100, p110, color);
        add_line(vertices, p101, p111, color);
        add_line(vertices, p001, p011, color);
    }
}
