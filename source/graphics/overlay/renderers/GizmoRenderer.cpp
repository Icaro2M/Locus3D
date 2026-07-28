/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/overlay/renderers/GizmoRenderer.h"

#include "graphics/common/GraphicsError.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace locus::graphics
{
    namespace
    {
        constexpr float Epsilon = 0.000001f;
        constexpr int RingSegments = 96;

        [[nodiscard]] glm::vec3 safe_normalize(
            const glm::vec3& value,
            const glm::vec3& fallback)
        {
            const float length = glm::length(value);
            if (length <= Epsilon)
            {
                return fallback;
            }

            return value / length;
        }

        [[nodiscard]] MeshVertex make_vertex(
            const glm::vec3& position,
            const ColorRGBA& color)
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

        void add_line(
            MeshUploadData& data,
            const glm::vec3& a,
            const glm::vec3& b,
            const ColorRGBA& color)
        {
            data.vertices.push_back(make_vertex(a, color));
            data.vertices.push_back(make_vertex(b, color));
        }

        [[nodiscard]] glm::vec3 axis_vector(GizmoVisualHandle handle)
        {
            switch (handle)
            {
            case GizmoVisualHandle::X:
                return { 1.0f, 0.0f, 0.0f };
            case GizmoVisualHandle::Y:
                return { 0.0f, 1.0f, 0.0f };
            case GizmoVisualHandle::Z:
                return { 0.0f, 0.0f, 1.0f };
            default:
                return { 0.0f, 0.0f, 0.0f };
            }
        }

        [[nodiscard]] bool is_single_axis(GizmoVisualHandle handle)
        {
            return handle == GizmoVisualHandle::X
                || handle == GizmoVisualHandle::Y
                || handle == GizmoVisualHandle::Z;
        }

        [[nodiscard]] bool is_plane_handle(GizmoVisualHandle handle)
        {
            return handle == GizmoVisualHandle::XY
                || handle == GizmoVisualHandle::XZ
                || handle == GizmoVisualHandle::YZ;
        }

        void plane_basis(
            GizmoVisualHandle handle,
            glm::vec3& u,
            glm::vec3& v)
        {
            switch (handle)
            {
            case GizmoVisualHandle::XY:
                u = { 1.0f, 0.0f, 0.0f };
                v = { 0.0f, 1.0f, 0.0f };
                break;
            case GizmoVisualHandle::XZ:
                u = { 1.0f, 0.0f, 0.0f };
                v = { 0.0f, 0.0f, 1.0f };
                break;
            case GizmoVisualHandle::YZ:
                u = { 0.0f, 1.0f, 0.0f };
                v = { 0.0f, 0.0f, 1.0f };
                break;
            default:
                u = { 1.0f, 0.0f, 0.0f };
                v = { 0.0f, 1.0f, 0.0f };
                break;
            }
        }

        void add_axis_arrow(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            float length,
            float radius,
            const ColorRGBA& color)
        {
            const glm::vec3 axis = axis_vector(handle);
            if (glm::length(axis) <= Epsilon)
            {
                return;
            }

            glm::vec3 sideA{ 0.0f, 1.0f, 0.0f };
            glm::vec3 sideB{ 0.0f, 0.0f, 1.0f };

            if (handle == GizmoVisualHandle::Y)
            {
                sideA = { 1.0f, 0.0f, 0.0f };
                sideB = { 0.0f, 0.0f, 1.0f };
            }
            else if (handle == GizmoVisualHandle::Z)
            {
                sideA = { 1.0f, 0.0f, 0.0f };
                sideB = { 0.0f, 1.0f, 0.0f };
            }

            const glm::vec3 end = axis * length;
            const glm::vec3 base = axis * std::max(length - radius * 3.0f, 0.0f);
            const float wing = radius * 1.7f;

            add_line(data, { 0.0f, 0.0f, 0.0f }, end, color);
            add_line(data, end, base + sideA * wing, color);
            add_line(data, end, base - sideA * wing, color);
            add_line(data, end, base + sideB * wing, color);
            add_line(data, end, base - sideB * wing, color);
        }

        void add_scale_handle(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            float length,
            float radius,
            const ColorRGBA& color)
        {
            const glm::vec3 axis = axis_vector(handle);
            if (glm::length(axis) <= Epsilon)
            {
                return;
            }

            const glm::vec3 center = axis * length;
            const glm::vec3 minPoint = center - glm::vec3{ radius };
            const glm::vec3 maxPoint = center + glm::vec3{ radius };

            add_line(data, { 0.0f, 0.0f, 0.0f }, center, color);

            const glm::vec3 p000{ minPoint.x, minPoint.y, minPoint.z };
            const glm::vec3 p001{ minPoint.x, minPoint.y, maxPoint.z };
            const glm::vec3 p010{ minPoint.x, maxPoint.y, minPoint.z };
            const glm::vec3 p011{ minPoint.x, maxPoint.y, maxPoint.z };
            const glm::vec3 p100{ maxPoint.x, minPoint.y, minPoint.z };
            const glm::vec3 p101{ maxPoint.x, minPoint.y, maxPoint.z };
            const glm::vec3 p110{ maxPoint.x, maxPoint.y, minPoint.z };
            const glm::vec3 p111{ maxPoint.x, maxPoint.y, maxPoint.z };

            add_line(data, p000, p100, color);
            add_line(data, p100, p101, color);
            add_line(data, p101, p001, color);
            add_line(data, p001, p000, color);
            add_line(data, p010, p110, color);
            add_line(data, p110, p111, color);
            add_line(data, p111, p011, color);
            add_line(data, p011, p010, color);
            add_line(data, p000, p010, color);
            add_line(data, p100, p110, color);
            add_line(data, p101, p111, color);
            add_line(data, p001, p011, color);
        }

        void add_plane_square(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            float offset,
            float size,
            const ColorRGBA& color)
        {
            glm::vec3 u{};
            glm::vec3 v{};
            plane_basis(handle, u, v);

            const glm::vec3 p0 = u * offset + v * offset;
            const glm::vec3 p1 = u * (offset + size) + v * offset;
            const glm::vec3 p2 = u * (offset + size) + v * (offset + size);
            const glm::vec3 p3 = u * offset + v * (offset + size);

            add_line(data, p0, p1, color);
            add_line(data, p1, p2, color);
            add_line(data, p2, p3, color);
            add_line(data, p3, p0, color);
            add_line(data, p0, p2, color);
        }

        void add_center(
            MeshUploadData& data,
            float radius,
            const ColorRGBA& color)
        {
            add_line(data, { -radius, 0.0f, 0.0f }, { radius, 0.0f, 0.0f }, color);
            add_line(data, { 0.0f, -radius, 0.0f }, { 0.0f, radius, 0.0f }, color);
            add_line(data, { 0.0f, 0.0f, -radius }, { 0.0f, 0.0f, radius }, color);
        }

        void add_rotation_ring(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            float radius,
            const ColorRGBA& color)
        {
            for (int i = 0; i < RingSegments; ++i)
            {
                const float a0 = (static_cast<float>(i) / RingSegments) * glm::two_pi<float>();
                const float a1 = (static_cast<float>(i + 1) / RingSegments) * glm::two_pi<float>();

                glm::vec3 p0{};
                glm::vec3 p1{};

                switch (handle)
                {
                case GizmoVisualHandle::X:
                    p0 = { 0.0f, std::cos(a0) * radius, std::sin(a0) * radius };
                    p1 = { 0.0f, std::cos(a1) * radius, std::sin(a1) * radius };
                    break;
                case GizmoVisualHandle::Y:
                    p0 = { std::cos(a0) * radius, 0.0f, std::sin(a0) * radius };
                    p1 = { std::cos(a1) * radius, 0.0f, std::sin(a1) * radius };
                    break;
                case GizmoVisualHandle::Z:
                case GizmoVisualHandle::View:
                    p0 = { std::cos(a0) * radius, std::sin(a0) * radius, 0.0f };
                    p1 = { std::cos(a1) * radius, std::sin(a1) * radius, 0.0f };
                    break;
                default:
                    return;
                }

                add_line(data, p0, p1, color);
            }
        }

        [[nodiscard]] bool matches_selection(
            const GizmoVisualSelection& selection,
            GizmoVisualMode mode,
            GizmoVisualHandle handle)
        {
            return selection.valid
                && selection.mode == mode
                && selection.handle == handle;
        }
    }

    GizmoRenderer::~GizmoRenderer()
    {
        destroy();
    }

    GizmoRenderer::GizmoRenderer(GizmoRenderer&& other) noexcept
    {
        *this = std::move(other);
    }

    GizmoRenderer& GizmoRenderer::operator=(GizmoRenderer&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        config_ = std::move(other.config_);
        drawData_ = other.drawData_;
        handles_ = std::move(other.handles_);
        submittedObjectCount_ = other.submittedObjectCount_;
        created_ = other.created_;

        rebind_objects();

        other.submittedObjectCount_ = 0;
        other.created_ = false;
        other.handles_.clear();

        return *this;
    }

    GraphicsResult<void> GizmoRenderer::create(
        const MeshUploader& uploader,
        const ShaderManager& shaderManager,
        const GizmoRendererConfig& config)
    {
        destroy();

        if (config.axisLength <= 0.0f
            || config.axisThickness <= 0.0f
            || config.planeSize <= 0.0f
            || config.planeOffset < 0.0f
            || config.centerRadius <= 0.0f
            || config.rotationRadius <= 0.0f
            || config.rotationThickness <= 0.0f
            || config.viewRingScale <= 0.0f
            || config.scaleHandleRadius <= 0.0f)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "GizmoRenderer received invalid configuration.");
        }

        const Shader* shader = shaderManager.find(config.shaderName);
        if (shader == nullptr)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceNotFound,
                "GizmoRenderer requires a loaded vertex-color shader.");
        }

        config_ = config;
        RenderObject::Id nextObjectId = config_.firstObjectId;

        const struct Entry
        {
            GizmoVisualMode mode;
            GizmoVisualHandle handle;
            bool viewFacing;
        } entries[] = {
            { GizmoVisualMode::Translate, GizmoVisualHandle::X, false },
            { GizmoVisualMode::Translate, GizmoVisualHandle::Y, false },
            { GizmoVisualMode::Translate, GizmoVisualHandle::Z, false },
            { GizmoVisualMode::Translate, GizmoVisualHandle::XY, false },
            { GizmoVisualMode::Translate, GizmoVisualHandle::XZ, false },
            { GizmoVisualMode::Translate, GizmoVisualHandle::YZ, false },
            { GizmoVisualMode::Translate, GizmoVisualHandle::XYZ, false },
            { GizmoVisualMode::Rotate, GizmoVisualHandle::X, false },
            { GizmoVisualMode::Rotate, GizmoVisualHandle::Y, false },
            { GizmoVisualMode::Rotate, GizmoVisualHandle::Z, false },
            { GizmoVisualMode::Rotate, GizmoVisualHandle::View, true },
            { GizmoVisualMode::Scale, GizmoVisualHandle::X, false },
            { GizmoVisualMode::Scale, GizmoVisualHandle::Y, false },
            { GizmoVisualMode::Scale, GizmoVisualHandle::Z, false },
            { GizmoVisualMode::Scale, GizmoVisualHandle::XY, false },
            { GizmoVisualMode::Scale, GizmoVisualHandle::XZ, false },
            { GizmoVisualMode::Scale, GizmoVisualHandle::YZ, false },
            { GizmoVisualMode::Scale, GizmoVisualHandle::XYZ, false },
        };

        handles_.reserve(std::size(entries));
        for (const Entry& entry : entries)
        {
            auto result = create_handle(
                uploader,
                shader,
                entry.mode,
                entry.handle,
                entry.viewFacing,
                nextObjectId);

            if (!result)
            {
                destroy();
                return result.error();
            }
        }

        created_ = true;
        update(drawData_);
        return {};
    }

    void GizmoRenderer::destroy()
    {
        for (HandleObject& handle : handles_)
        {
            for (GpuMesh& mesh : handle.meshes)
            {
                mesh.destroy();
            }

            for (RenderObject& object : handle.objects)
            {
                object.mesh = nullptr;
                object.shader = nullptr;
            }

            handle.currentObject = nullptr;
            handle.visible = false;
        }

        handles_.clear();
        submittedObjectCount_ = 0;
        created_ = false;
        config_ = {};
        drawData_ = {};
    }

    void GizmoRenderer::update(const GizmoDrawData& data)
    {
        drawData_ = data;
        submittedObjectCount_ = 0;

        if (!created_)
        {
            return;
        }

        const float visualScale = std::max(data.visualScale, 0.0001f);

        for (HandleObject& handle : handles_)
        {
            const bool visible =
                data.visible
                && data.mode != GizmoVisualMode::None
                && should_draw(data.mode, handle.mode, handle.handle);

            handle.visible = visible;
            handle.currentObject = nullptr;

            const GizmoVisualRole role =
                resolve_role(data, handle.mode, handle.handle);
            const std::size_t index = role_index(role);

            for (RenderObject& object : handle.objects)
            {
                object.transform.position = data.pivot;
                object.transform.rotation = handle.viewFacing
                    ? view_facing_rotation(data)
                    : data.orientation;
                object.transform.scale = { visualScale, visualScale, visualScale };
            }

            if (visible)
            {
                handle.currentObject = &handle.objects[index];
                ++submittedObjectCount_;
            }
        }
    }

    void GizmoRenderer::submit(RenderScene& scene) const
    {
        if (!created_)
        {
            return;
        }

        for (const HandleObject& handle : handles_)
        {
            if (handle.visible && handle.currentObject != nullptr)
            {
                scene.add_object(*handle.currentObject);
            }
        }
    }

    bool GizmoRenderer::is_valid() const
    {
        if (!created_ || handles_.empty())
        {
            return false;
        }

        for (const HandleObject& handle : handles_)
        {
            for (const RenderObject& object : handle.objects)
            {
                if (!object.is_drawable())
                {
                    return false;
                }
            }
        }

        return true;
    }

    const GizmoRendererConfig& GizmoRenderer::config() const
    {
        return config_;
    }

    const GizmoDrawData& GizmoRenderer::draw_data() const
    {
        return drawData_;
    }

    std::size_t GizmoRenderer::submitted_object_count() const
    {
        return submittedObjectCount_;
    }

    GraphicsResult<void> GizmoRenderer::create_handle(
        const MeshUploader& uploader,
        const Shader* shader,
        GizmoVisualMode mode,
        GizmoVisualHandle handle,
        bool viewFacing,
        RenderObject::Id& nextObjectId)
    {
        HandleObject entry;
        entry.mode = mode;
        entry.handle = handle;
        entry.viewFacing = viewFacing;

        const ColorRGBA normalColor = base_color(handle, config_);
        const ColorRGBA colors[] = {
            normalColor,
            config_.hoverColor,
            config_.activeColor,
            config_.disabledColor
        };

        for (std::size_t index = 0; index < RoleCount; ++index)
        {
            MeshUploadData meshData = build_mesh_data(
                mode,
                handle,
                config_,
                colors[index]);

            if (meshData.is_empty())
            {
                return GraphicsError::make(
                    GraphicsErrorCode::InvalidArgument,
                    "GizmoRenderer could not build handle geometry.");
            }

            auto meshResult = uploader.upload(meshData);
            if (!meshResult)
            {
                return meshResult.error();
            }

            entry.meshes[index] = meshResult.move_value();

            RenderObject object;
            object.id = nextObjectId++;
            object.name = config_.objectNamePrefix;
            object.mesh = &entry.meshes[index];
            object.shader = shader;
            object.layer = config_.layer;
            object.visibility.selectable = false;
            object.pickingId = PickingId::invalid();

            entry.objects[index] = std::move(object);
        }

        handles_.push_back(std::move(entry));

        HandleObject& stored = handles_.back();
        for (std::size_t index = 0; index < RoleCount; ++index)
        {
            stored.objects[index].mesh = &stored.meshes[index];
        }

        return {};
    }

    void GizmoRenderer::rebind_objects() noexcept
    {
        for (HandleObject& handle : handles_)
        {
            handle.currentObject = nullptr;

            for (std::size_t index = 0; index < RoleCount; ++index)
            {
                handle.objects[index].mesh = &handle.meshes[index];
            }
        }

        update(drawData_);
    }

    MeshUploadData GizmoRenderer::build_mesh_data(
        GizmoVisualMode mode,
        GizmoVisualHandle handle,
        const GizmoRendererConfig& config,
        const ColorRGBA& color)
    {
        MeshUploadData data;
        data.topology = PrimitiveTopology::Lines;
        data.usage = BufferUsage::Static;

        if (mode == GizmoVisualMode::Translate && is_single_axis(handle))
        {
            add_axis_arrow(data, handle, config.axisLength, config.axisThickness, color);
            return data;
        }

        if (mode == GizmoVisualMode::Scale && is_single_axis(handle))
        {
            add_scale_handle(data, handle, config.axisLength, config.scaleHandleRadius, color);
            return data;
        }

        if ((mode == GizmoVisualMode::Translate || mode == GizmoVisualMode::Scale)
            && is_plane_handle(handle))
        {
            add_plane_square(data, handle, config.planeOffset, config.planeSize, color);
            return data;
        }

        if ((mode == GizmoVisualMode::Translate || mode == GizmoVisualMode::Scale)
            && handle == GizmoVisualHandle::XYZ)
        {
            add_center(data, config.centerRadius, color);
            return data;
        }

        if (mode == GizmoVisualMode::Rotate)
        {
            const float radius = handle == GizmoVisualHandle::View
                ? config.rotationRadius * config.viewRingScale
                : config.rotationRadius;
            add_rotation_ring(data, handle, radius, color);
            return data;
        }

        return data;
    }

    ColorRGBA GizmoRenderer::base_color(
        GizmoVisualHandle handle,
        const GizmoRendererConfig& config)
    {
        switch (handle)
        {
        case GizmoVisualHandle::X:
            return config.xColor;
        case GizmoVisualHandle::Y:
            return config.yColor;
        case GizmoVisualHandle::Z:
            return config.zColor;
        case GizmoVisualHandle::XY:
        case GizmoVisualHandle::XZ:
        case GizmoVisualHandle::YZ:
            return config.planeColor;
        case GizmoVisualHandle::XYZ:
            return config.centerColor;
        case GizmoVisualHandle::View:
            return config.viewColor;
        case GizmoVisualHandle::None:
        default:
            return config.disabledColor;
        }
    }

    GizmoVisualRole GizmoRenderer::resolve_role(
        const GizmoDrawData& data,
        GizmoVisualMode mode,
        GizmoVisualHandle handle)
    {
        if (!data.enabled)
        {
            return GizmoVisualRole::Disabled;
        }

        if (matches_selection(data.active, mode, handle))
        {
            return GizmoVisualRole::Active;
        }

        if (matches_selection(data.hovered, mode, handle))
        {
            return GizmoVisualRole::Hovered;
        }

        return GizmoVisualRole::Normal;
    }

    bool GizmoRenderer::should_draw(
        GizmoVisualMode activeMode,
        GizmoVisualMode objectMode,
        GizmoVisualHandle handle)
    {
        if (activeMode == objectMode)
        {
            return true;
        }

        if (activeMode != GizmoVisualMode::Universal)
        {
            return false;
        }

        if (objectMode == GizmoVisualMode::Rotate)
        {
            return true;
        }

        if (objectMode == GizmoVisualMode::Translate)
        {
            return true;
        }

        if (objectMode == GizmoVisualMode::Scale)
        {
            return true;
        }

        return false;
    }

    std::size_t GizmoRenderer::role_index(GizmoVisualRole role) noexcept
    {
        switch (role)
        {
        case GizmoVisualRole::Normal:
            return 0;
        case GizmoVisualRole::Hovered:
            return 1;
        case GizmoVisualRole::Active:
            return 2;
        case GizmoVisualRole::Disabled:
            return 3;
        }

        return 0;
    }

    glm::quat GizmoRenderer::view_facing_rotation(
        const GizmoDrawData& data)
    {
        const glm::vec3 right = safe_normalize(data.viewRight, { 1.0f, 0.0f, 0.0f });
        const glm::vec3 up = safe_normalize(data.viewUp, { 0.0f, 1.0f, 0.0f });
        const glm::vec3 normal = safe_normalize(-data.viewDirection, { 0.0f, 0.0f, 1.0f });

        glm::mat3 basis{ 1.0f };
        basis[0] = right;
        basis[1] = up;
        basis[2] = normal;

        return glm::normalize(glm::quat_cast(basis));
    }
}
