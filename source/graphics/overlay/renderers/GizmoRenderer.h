/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/appearance/ViewportPalette.h"
#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace locus::graphics
{
    /**
     * @brief Graphics-only transform gizmo mode.
     *
     * This enum intentionally mirrors the visual surface of editor gizmo modes
     * without depending on editor headers or behavior.
     */
    enum class GizmoVisualMode
    {
        None,
        Translate,
        Rotate,
        Scale,
        Universal
    };

    /**
     * @brief Graphics-only transform gizmo handle identifier.
     */
    enum class GizmoVisualHandle
    {
        None,
        X,
        Y,
        Z,
        XY,
        XZ,
        YZ,
        XYZ,
        View
    };

    /**
     * @brief Visual role used to resolve handle appearance.
     */
    enum class GizmoVisualRole
    {
        Normal,
        Hovered,
        Active,
        Disabled
    };

    /**
     * @brief Identifies one hovered or active visual gizmo handle.
     */
    struct GizmoVisualSelection
    {
        /**
         * @brief True when mode and handle should be considered.
         */
        bool valid = false;

        /**
         * @brief Mode that owns the selected visual handle.
         */
        GizmoVisualMode mode = GizmoVisualMode::None;

        /**
         * @brief Selected visual handle.
         */
        GizmoVisualHandle handle = GizmoVisualHandle::None;

        /**
         * @brief Creates an empty selection.
         *
         * @return Invalid visual selection.
         */
        [[nodiscard]] static GizmoVisualSelection none()
        {
            return {};
        }

        /**
         * @brief Creates a valid visual selection.
         *
         * @param mode Selection mode.
         * @param handle Selection handle.
         * @return Valid visual selection.
         */
        [[nodiscard]] static GizmoVisualSelection make(
            GizmoVisualMode mode,
            GizmoVisualHandle handle)
        {
            GizmoVisualSelection selection;
            selection.valid = true;
            selection.mode = mode;
            selection.handle = handle;
            return selection;
        }
    };

    /**
     * @brief Complete graphics-side state needed to draw one transform gizmo.
     */
    struct GizmoDrawData
    {
        /**
         * @brief Active visual mode.
         */
        GizmoVisualMode mode = GizmoVisualMode::Translate;

        /**
         * @brief World-space pivot for all gizmo handles.
         */
        glm::vec3 pivot{ 0.0f, 0.0f, 0.0f };

        /**
         * @brief World-space orientation applied to non-view handles.
         */
        glm::quat orientation{ 1.0f, 0.0f, 0.0f, 0.0f };

        /**
         * @brief Uniform visual scale applied to base gizmo dimensions.
         */
        float visualScale = 1.0f;

        /**
         * @brief Camera view direction used by the view-facing rotation ring.
         */
        glm::vec3 viewDirection{ 0.0f, 0.0f, -1.0f };

        /**
         * @brief Camera right direction used by view-facing geometry.
         */
        glm::vec3 viewRight{ 1.0f, 0.0f, 0.0f };

        /**
         * @brief Camera up direction used by view-facing geometry.
         */
        glm::vec3 viewUp{ 0.0f, 1.0f, 0.0f };

        /**
         * @brief Hovered handle, if any.
         */
        GizmoVisualSelection hovered{};

        /**
         * @brief Active handle, if any. Active appearance overrides hover.
         */
        GizmoVisualSelection active{};

        /**
         * @brief True when the gizmo should be submitted.
         */
        bool visible = true;

        /**
         * @brief True when normal/hover/active colors should be used.
         */
        bool enabled = true;
    };

    /**
     * @brief Visual configuration for transform gizmo rendering.
     *
     * Dimension defaults match the editor transform gizmo hit-test defaults so
     * the visual handles can stay aligned with logical hit testing across the
     * application/editor boundary.
     */
    struct GizmoRendererConfig
    {
        float axisLength = 1.35f;
        float axisThickness = 0.06f;
        float planeSize = 0.32f;
        float planeOffset = 0.22f;
        float centerRadius = 0.12f;
        float rotationRadius = 1.05f;
        float rotationThickness = 0.06f;
        float viewRingScale = 1.12f;
        float scaleHandleRadius = 0.10f;

        RenderObject::Id firstObjectId = 1100;
        std::string objectNamePrefix = "TransformGizmo";
        RenderLayer layer = RenderLayer::Gizmo;
        std::string shaderName = "viewport/axis";

        ColorRGBA xColor{ 0.90f, 0.20f, 0.20f, 1.0f };
        ColorRGBA yColor{ 0.20f, 0.80f, 0.25f, 1.0f };
        ColorRGBA zColor{ 0.25f, 0.45f, 1.0f, 1.0f };
        ColorRGBA planeColor{ 0.95f, 0.85f, 0.24f, 1.0f };
        ColorRGBA centerColor{ 0.90f, 0.90f, 0.95f, 1.0f };
        ColorRGBA viewColor{ 0.72f, 0.72f, 0.78f, 1.0f };
        ColorRGBA hoverColor{ 1.0f, 0.92f, 0.25f, 1.0f };
        ColorRGBA activeColor{ 1.0f, 0.55f, 0.10f, 1.0f };
        ColorRGBA disabledColor{ 0.36f, 0.36f, 0.40f, 1.0f };
    };

    /**
     * @brief Renders graphics-only transform gizmo handles.
     *
     * GizmoRenderer owns GPU meshes and render objects only. It performs no
     * hit-testing, snapping, command execution, editor selection lookup, or
     * transform session management.
     */
    class GizmoRenderer
    {
    public:
        GizmoRenderer() = default;
        ~GizmoRenderer();

        GizmoRenderer(const GizmoRenderer&) = delete;
        GizmoRenderer& operator=(const GizmoRenderer&) = delete;

        GizmoRenderer(GizmoRenderer&& other) noexcept;
        GizmoRenderer& operator=(GizmoRenderer&& other) noexcept;

        /**
         * @brief Creates all static handle meshes and render objects.
         *
         * @param uploader Mesh uploader used to create GPU buffers.
         * @param shaderManager Shader manager used to resolve the gizmo shader.
         * @param config Visual dimensions, colors, object IDs, and shader name.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const MeshUploader& uploader,
            const ShaderManager& shaderManager,
            const GizmoRendererConfig& config = {});

        /**
         * @brief Releases all owned GPU resources.
         */
        void destroy();

        /**
         * @brief Updates draw state without recreating GPU resources.
         *
         * @param data Explicit graphics-side gizmo state.
         */
        void update(const GizmoDrawData& data);

        /**
         * @brief Submits currently visible gizmo render objects to a scene.
         *
         * @param scene Scene that receives gizmo objects.
         */
        void submit(RenderScene& scene) const;

        /**
         * @brief Checks whether every owned handle mesh is drawable.
         *
         * @return True after successful creation and before destroy().
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the active visual configuration.
         *
         * @return Renderer configuration.
         */
        [[nodiscard]] const GizmoRendererConfig& config() const;

        /**
         * @brief Returns the latest draw data.
         *
         * @return Current gizmo draw state.
         */
        [[nodiscard]] const GizmoDrawData& draw_data() const;

        /**
         * @brief Returns the number of objects currently submitted by submit().
         *
         * @return Visible object count.
         */
        [[nodiscard]] std::size_t submitted_object_count() const;

    private:
        static constexpr std::size_t RoleCount = 4;

        struct HandleObject
        {
            GizmoVisualMode mode = GizmoVisualMode::None;
            GizmoVisualHandle handle = GizmoVisualHandle::None;
            bool viewFacing = false;
            std::array<GpuMesh, RoleCount> meshes{};
            std::array<RenderObject, RoleCount> objects{};
            const RenderObject* currentObject = nullptr;
            bool visible = false;
        };

        [[nodiscard]] GraphicsResult<void> create_handle(
            const MeshUploader& uploader,
            const Shader* shader,
            GizmoVisualMode mode,
            GizmoVisualHandle handle,
            bool viewFacing,
            RenderObject::Id& nextObjectId);

        void rebind_objects() noexcept;

        [[nodiscard]] static MeshUploadData build_mesh_data(
            GizmoVisualMode mode,
            GizmoVisualHandle handle,
            const GizmoRendererConfig& config,
            const ColorRGBA& color);

        [[nodiscard]] static ColorRGBA base_color(
            GizmoVisualHandle handle,
            const GizmoRendererConfig& config);

        [[nodiscard]] static GizmoVisualRole resolve_role(
            const GizmoDrawData& data,
            GizmoVisualMode mode,
            GizmoVisualHandle handle);

        [[nodiscard]] static bool should_draw(
            GizmoVisualMode activeMode,
            GizmoVisualMode objectMode,
            GizmoVisualHandle handle);

        [[nodiscard]] static std::size_t role_index(
            GizmoVisualRole role) noexcept;

        [[nodiscard]] static glm::quat view_facing_rotation(
            const GizmoDrawData& data);

    private:
        GizmoRendererConfig config_{};
        GizmoDrawData drawData_{};
        std::vector<HandleObject> handles_{};
        std::size_t submittedObjectCount_ = 0;
        bool created_ = false;
    };
}
