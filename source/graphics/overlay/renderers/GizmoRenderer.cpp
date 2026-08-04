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
        constexpr int MinimumRadialSegments = 8;
        constexpr int MinimumRingMajorSegments = 16;
        constexpr int MinimumRingMinorSegments = 4;

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

        [[nodiscard]] ColorRGBA scaled_alpha(
            ColorRGBA color,
            float scale)
        {
            color.a = std::clamp(color.a * scale, 0.0f, 1.0f);
            return color;
        }

        [[nodiscard]] ColorRGBA mix_color(
            const ColorRGBA& a,
            const ColorRGBA& b,
            float t)
        {
            t = std::clamp(t, 0.0f, 1.0f);
            return {
                a.r + (b.r - a.r) * t,
                a.g + (b.g - a.g) * t,
                a.b + (b.b - a.b) * t,
                a.a + (b.a - a.a) * t
            };
        }

        [[nodiscard]] MeshVertex make_vertex(
            const glm::vec3& position,
            const glm::vec3& normal,
            const ColorRGBA& color)
        {
            MeshVertex vertex;
            vertex.position[0] = position.x;
            vertex.position[1] = position.y;
            vertex.position[2] = position.z;
            vertex.normal[0] = normal.x;
            vertex.normal[1] = normal.y;
            vertex.normal[2] = normal.z;
            vertex.color[0] = color.r;
            vertex.color[1] = color.g;
            vertex.color[2] = color.b;
            vertex.color[3] = color.a;
            return vertex;
        }

        std::uint32_t add_vertex(
            MeshUploadData& data,
            const glm::vec3& position,
            const glm::vec3& normal,
            const ColorRGBA& color)
        {
            data.vertices.push_back(make_vertex(position, normal, color));
            return static_cast<std::uint32_t>(data.vertices.size() - 1u);
        }

        void add_triangle_indices(
            MeshUploadData& data,
            std::uint32_t a,
            std::uint32_t b,
            std::uint32_t c)
        {
            data.indices.push_back(a);
            data.indices.push_back(b);
            data.indices.push_back(c);
        }

        void add_quad_indices(
            MeshUploadData& data,
            std::uint32_t a,
            std::uint32_t b,
            std::uint32_t c,
            std::uint32_t d)
        {
            add_triangle_indices(data, a, b, c);
            add_triangle_indices(data, a, c, d);
        }

        void add_quad(
            MeshUploadData& data,
            const glm::vec3& a,
            const glm::vec3& b,
            const glm::vec3& c,
            const glm::vec3& d,
            const ColorRGBA& color)
        {
            const glm::vec3 normal = safe_normalize(
                glm::cross(b - a, c - a),
                { 0.0f, 1.0f, 0.0f });
            const std::uint32_t ia = add_vertex(data, a, normal, color);
            const std::uint32_t ib = add_vertex(data, b, normal, color);
            const std::uint32_t ic = add_vertex(data, c, normal, color);
            const std::uint32_t id = add_vertex(data, d, normal, color);
            add_quad_indices(data, ia, ib, ic, id);
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

        [[nodiscard]] bool axis_basis(
            GizmoVisualHandle handle,
            glm::vec3& axis,
            glm::vec3& sideA,
            glm::vec3& sideB)
        {
            axis = axis_vector(handle);
            if (glm::length(axis) <= Epsilon)
            {
                return false;
            }

            if (handle == GizmoVisualHandle::X)
            {
                sideA = { 0.0f, 1.0f, 0.0f };
                sideB = { 0.0f, 0.0f, 1.0f };
            }
            else if (handle == GizmoVisualHandle::Y)
            {
                sideA = { 1.0f, 0.0f, 0.0f };
                sideB = { 0.0f, 0.0f, 1.0f };
            }
            else
            {
                sideA = { 1.0f, 0.0f, 0.0f };
                sideB = { 0.0f, 1.0f, 0.0f };
            }

            return true;
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

        void add_cylinder(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            float length,
            float radius,
            int segments,
            const ColorRGBA& color)
        {
            glm::vec3 axis{};
            glm::vec3 sideA{};
            glm::vec3 sideB{};
            if (!axis_basis(handle, axis, sideA, sideB)
                || length <= Epsilon
                || radius <= Epsilon)
            {
                return;
            }

            segments = std::max(segments, MinimumRadialSegments);
            const glm::vec3 start = axis * radius * 1.25f;
            const glm::vec3 end = axis * length;
            const std::uint32_t startCenter =
                add_vertex(data, start, -axis, color);
            const std::uint32_t endCenter =
                add_vertex(data, end, axis, color);

            std::vector<std::uint32_t> startRing;
            std::vector<std::uint32_t> endRing;
            startRing.reserve(static_cast<std::size_t>(segments));
            endRing.reserve(static_cast<std::size_t>(segments));

            for (int i = 0; i < segments; ++i)
            {
                const float angle =
                    (static_cast<float>(i) / static_cast<float>(segments))
                    * glm::two_pi<float>();
                const glm::vec3 radial =
                    sideA * std::cos(angle) + sideB * std::sin(angle);
                startRing.push_back(add_vertex(data, start + radial * radius, radial, color));
                endRing.push_back(add_vertex(data, end + radial * radius, radial, color));
            }

            for (int i = 0; i < segments; ++i)
            {
                const std::uint32_t next =
                    static_cast<std::uint32_t>((i + 1) % segments);
                const std::uint32_t current =
                    static_cast<std::uint32_t>(i);

                add_quad_indices(
                    data,
                    startRing[current],
                    endRing[current],
                    endRing[next],
                    startRing[next]);
                add_triangle_indices(data, startCenter, startRing[next], startRing[current]);
                add_triangle_indices(data, endCenter, endRing[current], endRing[next]);
            }
        }

        void add_cone(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            float baseDistance,
            float length,
            float radius,
            int segments,
            const ColorRGBA& color)
        {
            glm::vec3 axis{};
            glm::vec3 sideA{};
            glm::vec3 sideB{};
            if (!axis_basis(handle, axis, sideA, sideB)
                || length <= Epsilon
                || radius <= Epsilon)
            {
                return;
            }

            segments = std::max(segments, MinimumRadialSegments);
            const glm::vec3 base = axis * baseDistance;
            const glm::vec3 tip = axis * (baseDistance + length);
            const std::uint32_t baseCenter =
                add_vertex(data, base, -axis, color);
            const std::uint32_t tipIndex =
                add_vertex(data, tip, axis, color);

            std::vector<std::uint32_t> ring;
            ring.reserve(static_cast<std::size_t>(segments));

            for (int i = 0; i < segments; ++i)
            {
                const float angle =
                    (static_cast<float>(i) / static_cast<float>(segments))
                    * glm::two_pi<float>();
                const glm::vec3 radial =
                    sideA * std::cos(angle) + sideB * std::sin(angle);
                const glm::vec3 normal =
                    safe_normalize(radial * length + axis * radius, radial);
                ring.push_back(add_vertex(data, base + radial * radius, normal, color));
            }

            for (int i = 0; i < segments; ++i)
            {
                const std::uint32_t next =
                    static_cast<std::uint32_t>((i + 1) % segments);
                const std::uint32_t current =
                    static_cast<std::uint32_t>(i);

                add_triangle_indices(data, ring[current], tipIndex, ring[next]);
                add_triangle_indices(data, baseCenter, ring[next], ring[current]);
            }
        }

        void add_box(
            MeshUploadData& data,
            const glm::vec3& center,
            float size,
            const ColorRGBA& color)
        {
            const glm::vec3 half{ size * 0.5f };
            const glm::vec3 minPoint = center - half;
            const glm::vec3 maxPoint = center + half;

            const glm::vec3 p000{ minPoint.x, minPoint.y, minPoint.z };
            const glm::vec3 p001{ minPoint.x, minPoint.y, maxPoint.z };
            const glm::vec3 p010{ minPoint.x, maxPoint.y, minPoint.z };
            const glm::vec3 p011{ minPoint.x, maxPoint.y, maxPoint.z };
            const glm::vec3 p100{ maxPoint.x, minPoint.y, minPoint.z };
            const glm::vec3 p101{ maxPoint.x, minPoint.y, maxPoint.z };
            const glm::vec3 p110{ maxPoint.x, maxPoint.y, minPoint.z };
            const glm::vec3 p111{ maxPoint.x, maxPoint.y, maxPoint.z };

            add_quad(data, p000, p001, p011, p010, color);
            add_quad(data, p100, p110, p111, p101, color);
            add_quad(data, p000, p100, p101, p001, color);
            add_quad(data, p010, p011, p111, p110, color);
            add_quad(data, p000, p010, p110, p100, color);
            add_quad(data, p001, p101, p111, p011, color);
        }

        void add_axis_arrow(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            const GizmoRendererConfig& config,
            const ColorRGBA& color)
        {
            const float coneLength =
                std::min(config.arrowLength, config.axisLength * 0.45f);
            const float shaftLength =
                std::max(config.axisLength - coneLength, config.shaftRadius * 2.0f);

            add_cylinder(
                data,
                handle,
                shaftLength,
                config.shaftRadius,
                config.radialSegments,
                color);
            add_cone(
                data,
                handle,
                shaftLength,
                config.axisLength - shaftLength,
                config.arrowRadius,
                config.radialSegments,
                color);
        }

        void add_scale_handle(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            const GizmoRendererConfig& config,
            const ColorRGBA& color)
        {
            const glm::vec3 axis = axis_vector(handle);
            if (glm::length(axis) <= Epsilon)
            {
                return;
            }

            const float halfCube = config.scaleCubeSize * 0.5f;
            add_cylinder(
                data,
                handle,
                std::max(config.axisLength - halfCube, config.shaftRadius * 2.0f),
                config.shaftRadius,
                config.radialSegments,
                color);
            add_box(data, axis * config.axisLength, config.scaleCubeSize, color);
        }

        void add_plane_square(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            float offset,
            float size,
            float borderWidth,
            const ColorRGBA& fillColor,
            const ColorRGBA& color)
        {
            glm::vec3 u{};
            glm::vec3 v{};
            plane_basis(handle, u, v);

            const glm::vec3 p0 = u * offset + v * offset;
            const glm::vec3 p1 = u * (offset + size) + v * offset;
            const glm::vec3 p2 = u * (offset + size) + v * (offset + size);
            const glm::vec3 p3 = u * offset + v * (offset + size);

            add_quad(data, p0, p1, p2, p3, fillColor);

            const float bw = std::min(borderWidth, size * 0.35f);
            add_quad(data, p0 - u * bw - v * bw, p1 + u * bw - v * bw, p1 + u * bw, p0 - u * bw, color);
            add_quad(data, p1 + u * bw, p2 + u * bw + v * bw, p2, p1, color);
            add_quad(data, p3 - u * bw, p2, p2 + u * bw + v * bw, p3 - u * bw + v * bw, color);
            add_quad(data, p0 - u * bw - v * bw, p0 - u * bw, p3 - u * bw, p3 - u * bw + v * bw, color);
        }

        void add_uv_sphere(
            MeshUploadData& data,
            float radius,
            int segments,
            const ColorRGBA& color)
        {
            segments = std::max(segments, MinimumRadialSegments);
            const int rings = std::max(segments / 2, 4);
            const std::uint32_t base =
                static_cast<std::uint32_t>(data.vertices.size());

            for (int ring = 0; ring <= rings; ++ring)
            {
                const float v = static_cast<float>(ring) / static_cast<float>(rings);
                const float phi = v * glm::pi<float>();
                const float y = std::cos(phi);
                const float r = std::sin(phi);

                for (int segment = 0; segment <= segments; ++segment)
                {
                    const float u = static_cast<float>(segment) / static_cast<float>(segments);
                    const float theta = u * glm::two_pi<float>();
                    const glm::vec3 normal{
                        r * std::cos(theta),
                        y,
                        r * std::sin(theta)
                    };
                    add_vertex(data, normal * radius, normal, color);
                }
            }

            const int stride = segments + 1;
            for (int ring = 0; ring < rings; ++ring)
            {
                for (int segment = 0; segment < segments; ++segment)
                {
                    const std::uint32_t a =
                        base + static_cast<std::uint32_t>(ring * stride + segment);
                    const std::uint32_t b = a + 1u;
                    const std::uint32_t c =
                        base + static_cast<std::uint32_t>((ring + 1) * stride + segment + 1);
                    const std::uint32_t d =
                        base + static_cast<std::uint32_t>((ring + 1) * stride + segment);
                    add_quad_indices(data, a, b, c, d);
                }
            }
        }

        void add_torus(
            MeshUploadData& data,
            GizmoVisualHandle handle,
            float radius,
            float tubeRadius,
            int majorSegments,
            int minorSegments,
            const ColorRGBA& color)
        {
            majorSegments = std::max(majorSegments, MinimumRingMajorSegments);
            minorSegments = std::max(minorSegments, MinimumRingMinorSegments);
            const std::uint32_t base =
                static_cast<std::uint32_t>(data.vertices.size());

            for (int major = 0; major < majorSegments; ++major)
            {
                const float u =
                    (static_cast<float>(major) / static_cast<float>(majorSegments))
                    * glm::two_pi<float>();
                const glm::vec3 radial{
                    std::cos(u),
                    std::sin(u),
                    0.0f
                };

                for (int minor = 0; minor < minorSegments; ++minor)
                {
                    const float v =
                        (static_cast<float>(minor) / static_cast<float>(minorSegments))
                        * glm::two_pi<float>();
                    const glm::vec3 normal{
                        radial.x * std::cos(v),
                        radial.y * std::cos(v),
                        std::sin(v)
                    };
                    const glm::vec3 xyPosition =
                        radial * (radius + tubeRadius * std::cos(v))
                        + glm::vec3{ 0.0f, 0.0f, tubeRadius * std::sin(v) };

                    glm::vec3 position = xyPosition;
                    glm::vec3 orientedNormal = normal;
                    if (handle == GizmoVisualHandle::X)
                    {
                        position = { xyPosition.z, xyPosition.x, xyPosition.y };
                        orientedNormal = { normal.z, normal.x, normal.y };
                    }
                    else if (handle == GizmoVisualHandle::Y)
                    {
                        position = { xyPosition.x, xyPosition.z, xyPosition.y };
                        orientedNormal = { normal.x, normal.z, normal.y };
                    }

                    add_vertex(data, position, orientedNormal, color);
                }
            }

            for (int major = 0; major < majorSegments; ++major)
            {
                const int nextMajor = (major + 1) % majorSegments;
                for (int minor = 0; minor < minorSegments; ++minor)
                {
                    const int nextMinor = (minor + 1) % minorSegments;
                    const std::uint32_t a =
                        base + static_cast<std::uint32_t>(major * minorSegments + minor);
                    const std::uint32_t b =
                        base + static_cast<std::uint32_t>(nextMajor * minorSegments + minor);
                    const std::uint32_t c =
                        base + static_cast<std::uint32_t>(nextMajor * minorSegments + nextMinor);
                    const std::uint32_t d =
                        base + static_cast<std::uint32_t>(major * minorSegments + nextMinor);
                    add_quad_indices(data, a, b, c, d);
                }
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
            || config.shaftRadius <= 0.0f
            || config.arrowLength <= 0.0f
            || config.arrowRadius <= 0.0f
            || config.planeSize <= 0.0f
            || config.planeOffset < 0.0f
            || config.planeBorderWidth <= 0.0f
            || config.centerRadius <= 0.0f
            || config.centerSize <= 0.0f
            || config.rotationRadius <= 0.0f
            || config.rotationThickness <= 0.0f
            || config.rotationTubeRadius <= 0.0f
            || config.viewRingScale <= 0.0f
            || config.scaleHandleRadius <= 0.0f
            || config.scaleCubeSize <= 0.0f
            || config.radialSegments < MinimumRadialSegments
            || config.ringMajorSegments < MinimumRingMajorSegments
            || config.ringMinorSegments < MinimumRingMinorSegments)
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

    MeshUploadData GizmoRenderer::build_handle_mesh_data(
        GizmoVisualMode mode,
        GizmoVisualHandle handle,
        const GizmoRendererConfig& config,
        GizmoVisualRole role)
    {
        ColorRGBA color = base_color(handle, config);

        switch (role)
        {
        case GizmoVisualRole::Hovered:
            color = config.hoverColor;
            break;
        case GizmoVisualRole::Active:
            color = config.activeColor;
            break;
        case GizmoVisualRole::Disabled:
            color = config.disabledColor;
            break;
        case GizmoVisualRole::Normal:
        default:
            break;
        }

        if (is_plane_handle(handle))
        {
            switch (role)
            {
            case GizmoVisualRole::Hovered:
                color.a = 1.0f;
                break;
            case GizmoVisualRole::Active:
                color.a = 1.0f;
                break;
            case GizmoVisualRole::Disabled:
                color.a = 0.42f;
                break;
            case GizmoVisualRole::Normal:
            default:
                color.a = 0.82f;
                break;
            }
        }
        else if (role == GizmoVisualRole::Disabled)
        {
            color.a = 0.62f;
        }

        return build_mesh_data(mode, handle, config, color);
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

        for (std::size_t index = 0; index < RoleCount; ++index)
        {
            MeshUploadData meshData = build_handle_mesh_data(
                mode,
                handle,
                config_,
                static_cast<GizmoVisualRole>(index));

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
        data.topology = PrimitiveTopology::Triangles;
        data.usage = BufferUsage::Static;

        if (mode == GizmoVisualMode::Translate && is_single_axis(handle))
        {
            add_axis_arrow(data, handle, config, color);
            return data;
        }

        if (mode == GizmoVisualMode::Scale && is_single_axis(handle))
        {
            add_scale_handle(data, handle, config, color);
            return data;
        }

        if ((mode == GizmoVisualMode::Translate || mode == GizmoVisualMode::Scale)
            && is_plane_handle(handle))
        {
            const ColorRGBA fillColor = scaled_alpha(color, 0.42f);
            add_plane_square(
                data,
                handle,
                config.planeOffset,
                config.planeSize,
                config.planeBorderWidth,
                fillColor,
                color);
            return data;
        }

        if (mode == GizmoVisualMode::Translate
            && handle == GizmoVisualHandle::XYZ)
        {
            add_uv_sphere(data, config.centerSize, config.radialSegments, color);
            return data;
        }

        if (mode == GizmoVisualMode::Scale
            && handle == GizmoVisualHandle::XYZ)
        {
            add_box(data, { 0.0f, 0.0f, 0.0f }, config.centerSize * 1.45f, color);
            return data;
        }

        if (mode == GizmoVisualMode::Rotate)
        {
            const float radius = handle == GizmoVisualHandle::View
                ? config.rotationRadius * config.viewRingScale
                : config.rotationRadius;
            const float tubeRadius = handle == GizmoVisualHandle::View
                ? config.rotationTubeRadius * 1.35f
                : config.rotationTubeRadius;
            add_torus(
                data,
                handle,
                radius,
                tubeRadius,
                config.ringMajorSegments,
                config.ringMinorSegments,
                color);
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
            return mix_color(config.xColor, config.yColor, 0.5f);
        case GizmoVisualHandle::XZ:
            return mix_color(config.xColor, config.zColor, 0.5f);
        case GizmoVisualHandle::YZ:
            return mix_color(config.yColor, config.zColor, 0.5f);
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
