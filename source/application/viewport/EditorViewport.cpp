/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/viewport/EditorViewport.h"

#include "application/document/DocumentSession.h"
#include "editor/EditorTypes.h"
#include "editor/Editor.h"
#include "editor/gizmo/GizmoAxis.h"
#include "editor/gizmo/GizmoMode.h"
#include "editor/gizmo/GizmoState.h"
#include "editor/render/MeshNodeRenderAdapter.h"
#include "editor/render/OverlayRenderAdapter.h"
#include "editor/render/PreviewRenderAdapter.h"
#include "editor/render/SelectionRenderAdapter.h"
#include "editor/render/TopologyOverlayAdapter.h"
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionState.h"
#include "editor/sync/EditorSync.h"
#include "editor/tools/core/ToolContext.h"
#include "editor/tools/mesh/core/MeshDragOperationTool.h"
#include "editor/tools/transform/TransformTool.h"
#include "graphics/common/GraphicsError.h"
#include "graphics/appearance/ViewportPalette.h"
#include "graphics/primitives/PrimitiveMeshConverter.h"
#include "graphics/primitives/SurfaceOverlay.h"
#include "graphics/scene/RenderObject.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <fstream>
#include <system_error>
#include <string>
#include <utility>
#include <vector>

namespace locus::application {

    namespace {

        constexpr float OrbitRadiansPerPixel = 0.005f;
        constexpr float PerspectivePanDistancePerPixel = 0.0015f;
        constexpr float ZoomDistanceRatio = 0.1f;
        constexpr float MinimumZoomStep = 0.05f;
        constexpr float MinPerspectiveFovRadians = 0.001f;
        constexpr float MinFramingDistance = 0.05f;
        constexpr float VisualScalePixels = 96.0f;

        [[nodiscard]] float safe_fov(float fov) noexcept
        {
            return std::max(fov, MinPerspectiveFovRadians);
        }

        [[nodiscard]] float orthographic_height_for_distance(
            float distance,
            float fovRadians) noexcept
        {
            return std::clamp(
                2.0f * std::max(distance, MinFramingDistance)
                    * std::tan(safe_fov(fovRadians) * 0.5f),
                graphics::Projection::min_orthographic_height(),
                graphics::Projection::max_orthographic_height());
        }

        [[nodiscard]] float perspective_distance_for_height(
            float height,
            float fovRadians) noexcept
        {
            const float tangent = std::tan(safe_fov(fovRadians) * 0.5f);
            if (tangent <= 0.000001f) {
                return MinFramingDistance;
            }

            return std::max(
                height / (2.0f * tangent),
                MinFramingDistance);
        }

        [[nodiscard]] glm::vec3 canonical_forward(
            ViewOrientation orientation) noexcept
        {
            switch (orientation) {
            case ViewOrientation::Front:
                return { 0.0f, 0.0f, -1.0f };
            case ViewOrientation::Back:
                return { 0.0f, 0.0f, 1.0f };
            case ViewOrientation::Left:
                return { -1.0f, 0.0f, 0.0f };
            case ViewOrientation::Right:
                return { 1.0f, 0.0f, 0.0f };
            case ViewOrientation::Top:
                return { 0.0f, -1.0f, 0.0f };
            case ViewOrientation::Bottom:
                return { 0.0f, 1.0f, 0.0f };
            case ViewOrientation::User:
                break;
            }

            return { 0.0f, 0.0f, -1.0f };
        }

        [[nodiscard]] glm::vec3 canonical_up(
            ViewOrientation orientation) noexcept
        {
            switch (orientation) {
            case ViewOrientation::Top:
                return { 0.0f, 0.0f, -1.0f };
            case ViewOrientation::Bottom:
                return { 0.0f, 0.0f, 1.0f };
            case ViewOrientation::User:
            case ViewOrientation::Front:
            case ViewOrientation::Back:
            case ViewOrientation::Left:
            case ViewOrientation::Right:
                return { 0.0f, 1.0f, 0.0f };
            }

            return { 0.0f, 1.0f, 0.0f };
        }

        [[nodiscard]] ApplicationResult<void> graphics_failure(
            ApplicationErrorCode code,
            const char* operation,
            const graphics::GraphicsError& error)
        {
            std::string message = operation;

            if (!error.message.empty()) {
                message += ": ";
                message += error.message;
            }

            return ApplicationError::make(code, std::move(message));
        }

        void append_startup_log(const char* message)
        {
            std::ofstream stream(
                "locus3d_startup.log",
                std::ios::app);

            if (stream.is_open()) {
                stream << message << '\n';
            }
        }

        [[nodiscard]] bool shader_root_has_viewport_assets(
            const std::filesystem::path& shaderRoot)
        {
            std::error_code error;
            return std::filesystem::exists(
                shaderRoot / "viewport" / "grid_vert.glsl",
                error) &&
                std::filesystem::exists(
                    shaderRoot / "viewport" / "point_marker_vert.glsl",
                    error);
        }

        void append_surface_overlay_batch(
            graphics::SurfaceOverlayBatch& output,
            const graphics::SurfaceOverlayBatch& input)
        {
            if (input.empty()) {
                return;
            }

            if (output.vertices.empty() && output.indices.empty()) {
                output.modelMatrix = input.modelMatrix;
            }

            const std::uint32_t base =
                static_cast<std::uint32_t>(output.vertices.size());

            output.vertices.reserve(output.vertices.size() + input.vertices.size());
            output.indices.reserve(output.indices.size() + input.indices.size());

            output.vertices.insert(
                output.vertices.end(),
                input.vertices.begin(),
                input.vertices.end());

            for (const std::uint32_t index : input.indices) {
                output.indices.push_back(base + index);
            }
        }

        [[nodiscard]] graphics::ScreenSpaceLineBatch make_occluded_lines(
            graphics::ScreenSpaceLineBatch input)
        {
            for (graphics::ScreenSpaceLine& line : input.lines) {
                line.color.a *= 0.26f;
            }

            return input;
        }

        [[nodiscard]] std::filesystem::path resolve_shader_root(
            const std::filesystem::path& shaderRoot)
        {
            if (shaderRoot.is_absolute() &&
                shader_root_has_viewport_assets(shaderRoot)) {
                return shaderRoot;
            }

            std::error_code error;
            std::filesystem::path current =
                std::filesystem::current_path(error);

            if (error) {
                return shaderRoot;
            }

            for (;;) {
                const std::filesystem::path candidate =
                    shaderRoot.is_absolute()
                    ? shaderRoot
                    : current / shaderRoot;

                if (shader_root_has_viewport_assets(candidate)) {
                    return candidate;
                }

                if (!current.has_parent_path() ||
                    current.parent_path() == current) {
                    break;
                }

                current = current.parent_path();
            }

            return shaderRoot;
        }

        [[nodiscard]] graphics::GizmoVisualMode to_visual_mode(
            editor::GizmoMode mode) noexcept
        {
            switch (mode) {
            case editor::GizmoMode::None:
                return graphics::GizmoVisualMode::None;
            case editor::GizmoMode::Translate:
                return graphics::GizmoVisualMode::Translate;
            case editor::GizmoMode::Rotate:
                return graphics::GizmoVisualMode::Rotate;
            case editor::GizmoMode::Scale:
                return graphics::GizmoVisualMode::Scale;
            case editor::GizmoMode::Universal:
                return graphics::GizmoVisualMode::Universal;
            }

            return graphics::GizmoVisualMode::None;
        }

        [[nodiscard]] graphics::GizmoVisualHandle to_visual_handle(
            editor::GizmoAxis axis) noexcept
        {
            switch (axis) {
            case editor::GizmoAxis::None:
                return graphics::GizmoVisualHandle::None;
            case editor::GizmoAxis::X:
                return graphics::GizmoVisualHandle::X;
            case editor::GizmoAxis::Y:
                return graphics::GizmoVisualHandle::Y;
            case editor::GizmoAxis::Z:
                return graphics::GizmoVisualHandle::Z;
            case editor::GizmoAxis::XY:
                return graphics::GizmoVisualHandle::XY;
            case editor::GizmoAxis::XZ:
                return graphics::GizmoVisualHandle::XZ;
            case editor::GizmoAxis::YZ:
                return graphics::GizmoVisualHandle::YZ;
            case editor::GizmoAxis::XYZ:
                return graphics::GizmoVisualHandle::XYZ;
            case editor::GizmoAxis::View:
                return graphics::GizmoVisualHandle::View;
            }

            return graphics::GizmoVisualHandle::None;
        }

        [[nodiscard]] graphics::GizmoVisualSelection to_visual_selection(
            const editor::GizmoHit& hit) noexcept
        {
            if (!hit.is_valid()) {
                return graphics::GizmoVisualSelection::none();
            }

            return graphics::GizmoVisualSelection::make(
                to_visual_mode(hit.mode),
                to_visual_handle(hit.axis));
        }

        [[nodiscard]] editor::ToolContext make_tool_context(
            DocumentSession& document)
        {
            return editor::ToolContext(
                document.editor(),
                document.command_dispatcher(),
                document.history(),
                document.editor_sync().picking_sync());
        }

        [[nodiscard]] editor::TransformTool* active_transform_tool(
            DocumentSession& document)
        {
            editor::ITool* tool =
                document.tool_manager().active_tool();

            return dynamic_cast<editor::TransformTool*>(tool);
        }

        [[nodiscard]] const editor::MeshDragOperationTool*
        active_mesh_drag_tool(
            const DocumentSession& document)
        {
            const editor::ITool* tool =
                document.tool_manager().active_tool();

            return dynamic_cast<const editor::MeshDragOperationTool*>(tool);
        }

        [[nodiscard]] editor::SceneNodeId operation_preview_target(
            const DocumentSession& document)
        {
            const editor::MeshDragOperationTool* tool =
                active_mesh_drag_tool(document);

            if (tool == nullptr || !tool->has_operation_preview()) {
                return {};
            }

            return tool->mesh_session().target().nodeId;
        }

        void append_mesh_component_overlays(
            const editor::Editor& editor,
            const graphics::Shader* shader,
            const graphics::MeshUploader& uploader,
            std::deque<graphics::GpuMesh>& meshes,
            std::vector<graphics::RenderObject>& objects)
        {
            const editor::SceneNodeId activeMesh =
                editor.selection().mesh().active_mesh();

            if (!activeMesh.is_valid()) {
                return;
            }

            const editor::MeshNode* meshNode =
                editor.scene().find_mesh(activeMesh);

            if (meshNode == nullptr || shader == nullptr) {
                return;
            }

            editor::OverlayRenderOptions overlayOptions{};
            overlayOptions.includeWireframe = false;
            overlayOptions.includeSelectedEdges = false;
            overlayOptions.includeHoveredEdge = false;

            editor::OverlayGeometry overlay =
                editor::OverlayRenderAdapter::build_mesh_overlay(
                    *meshNode,
                    editor.selection().mesh(),
                    overlayOptions);

            if (!overlay.has_geometry()) {
                return;
            }

            objects.reserve(objects.size() + overlay.groups.size());

            for (const editor::OverlayPrimitiveGroup& group
                : overlay.groups) {
                if (!group.has_geometry()) {
                    continue;
                }

                graphics::MeshUploadData uploadData =
                    graphics::PrimitiveMeshConverter::to_upload_data(
                        group.mesh,
                        graphics::BufferUsage::Dynamic);

                auto uploadResult = uploader.upload(uploadData);
                if (!uploadResult) {
                    continue;
                }

                meshes.push_back(uploadResult.move_value());

                editor::MeshNodeRenderOptions options{};
                options.shader = shader;
                options.layer = graphics::RenderLayer::Overlay;

                graphics::RenderObject object =
                    editor::MeshNodeRenderAdapter::build_render_object(
                        *meshNode,
                        &meshes.back(),
                        options);

                object.name = "Mesh component overlay";
                object.pickingId = graphics::PickingId::invalid();
                object.visibility.selectable = false;
                object.selected = false;
                object.hovered = false;

                objects.push_back(std::move(object));
            }
        }

        void append_operation_preview(
            const DocumentSession& document,
            const graphics::Shader* shader,
            const graphics::MeshUploader& uploader,
            std::deque<graphics::GpuMesh>& meshes,
            std::vector<graphics::RenderObject>& objects)
        {
            const editor::MeshDragOperationTool* tool =
                active_mesh_drag_tool(document);

            if (tool == nullptr || !tool->has_operation_preview()) {
                return;
            }

            const editor::MeshToolTarget& target =
                tool->mesh_session().target();

            const editor::MeshNode* meshNode =
                document.editor().scene().find_mesh(target.nodeId);

            if (meshNode == nullptr || shader == nullptr) {
                return;
            }

            editor::PreviewRenderOptions options{};
            options.solidShader = shader;
            options.wireShader = shader;
            options.solidObjectId = 0;
            options.wireObjectId = 0;

            const kernel::modeling::OperationPreview& preview =
                tool->operation_preview();

            graphics::MeshUploadData solidUpload =
                editor::PreviewRenderAdapter::build_solid_upload_data(
                    preview,
                    options);
            graphics::MeshUploadData wireUpload =
                editor::PreviewRenderAdapter::build_wire_upload_data(
                    preview,
                    options);

            if (solidUpload.is_empty() && wireUpload.is_empty()) {
                return;
            }

            const graphics::GpuMesh* solidMesh = nullptr;
            const graphics::GpuMesh* wireMesh = nullptr;

            if (!solidUpload.is_empty()) {
                auto uploadResult = uploader.upload(solidUpload);
                if (uploadResult) {
                    meshes.push_back(uploadResult.move_value());
                    solidMesh = &meshes.back();
                }
            }

            if (!wireUpload.is_empty()) {
                auto uploadResult = uploader.upload(wireUpload);
                if (uploadResult) {
                    meshes.push_back(uploadResult.move_value());
                    wireMesh = &meshes.back();
                }
            }

            editor::PreviewRenderObjects previewObjects =
                editor::PreviewRenderAdapter::build_render_objects(
                    *meshNode,
                    preview,
                    solidMesh,
                    wireMesh,
                    options);

            objects.reserve(objects.size() + 2u);

            if (previewObjects.hasSolid) {
                previewObjects.solid.pickingId =
                    graphics::PickingId::invalid();
                previewObjects.solid.visibility.selectable = false;
                objects.push_back(std::move(previewObjects.solid));
            }

            if (previewObjects.hasWire) {
                previewObjects.wire.pickingId =
                    graphics::PickingId::invalid();
                previewObjects.wire.visibility.selectable = false;
                objects.push_back(std::move(previewObjects.wire));
            }
        }

    } // namespace

    EditorViewport::~EditorViewport()
    {
        shutdown();
    }

    ApplicationResult<void> EditorViewport::initialize(
        std::int32_t framebufferWidth,
        std::int32_t framebufferHeight,
        const std::filesystem::path& shaderRoot)
    {
        if (initialized_) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "EditorViewport is already initialized.");
        }

        if (shaderRoot.empty()) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidConfiguration,
                "EditorViewport shader root cannot be empty.");
        }

        viewport_.resize(framebufferWidth, framebufferHeight);
        apply_orbit_rig_to_camera();

        shaderManager_.set_shader_root(resolve_shader_root(shaderRoot));
        append_startup_log("EditorViewport: shader root resolved");

        auto gridShaderResult = shaderManager_.load(
            "viewport/grid",
            "viewport/grid_vert.glsl",
            "viewport/grid_frag.glsl");

        if (!gridShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the viewport grid shader",
                gridShaderResult.error());
        }
        append_startup_log("EditorViewport: grid shader loaded");

        auto axisShaderResult = shaderManager_.load(
            "viewport/axis",
            "viewport/axis_vert.glsl",
            "viewport/axis_frag.glsl");

        if (!axisShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the viewport axis shader",
                axisShaderResult.error());
        }
        append_startup_log("EditorViewport: axis shader loaded");

        auto documentShaderResult = shaderManager_.load(
            "viewport/document",
            "basic/vertex.glsl",
            "basic/fragment.glsl");

        if (!documentShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the document scene shader",
                documentShaderResult.error());
        }

        documentShader_ = documentShaderResult.value();
        append_startup_log("EditorViewport: document shader loaded");

        auto topologyShaderResult = shaderManager_.load(
            "viewport/screen_space_line",
            "viewport/screen_space_line_vert.glsl",
            "viewport/screen_space_line_frag.glsl");

        if (!topologyShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the topology overlay line shader",
                topologyShaderResult.error());
        }
        append_startup_log("EditorViewport: line shader loaded");

        const auto topologyLineResult =
            topologyLineRenderer_.create(shaderManager_);

        if (!topologyLineResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the topology overlay line renderer",
                topologyLineResult.error());
        }
        append_startup_log("EditorViewport: line renderer created");

        auto pointMarkerShaderResult = shaderManager_.load(
            "viewport/point_marker",
            "viewport/point_marker_vert.glsl",
            "viewport/point_marker_frag.glsl");

        if (!pointMarkerShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the topology overlay point marker shader",
                pointMarkerShaderResult.error());
        }
        append_startup_log("EditorViewport: point marker shader loaded");

        append_startup_log("EditorViewport: point marker renderer deferred");

        auto surfaceOverlayShaderResult = shaderManager_.load(
            "viewport/surface_overlay",
            "viewport/surface_overlay_vert.glsl",
            "viewport/surface_overlay_frag.glsl");

        if (!surfaceOverlayShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the topology surface overlay shader",
                surfaceOverlayShaderResult.error());
        }
        append_startup_log("EditorViewport: surface overlay shader loaded");

        const auto surfaceOverlayRendererResult =
            topologySurfaceRenderer_.create(shaderManager_);

        if (!surfaceOverlayRendererResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the topology surface overlay renderer",
                surfaceOverlayRendererResult.error());
        }
        append_startup_log("EditorViewport: surface overlay renderer created");

        auto pickingShaderResult = shaderManager_.load(
            "picking/object",
            "picking/picking_vert.glsl",
            "picking/picking_frag.glsl");

        if (!pickingShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the viewport picking shader",
                pickingShaderResult.error());
        }
        append_startup_log("EditorViewport: picking shader loaded");

        const auto pickingRendererResult =
            pickingRenderer_.create(shaderManager_);

        if (!pickingRendererResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the viewport picking renderer",
                pickingRendererResult.error());
        }
        append_startup_log("EditorViewport: picking renderer created");

        auto selectionMaskShaderResult = shaderManager_.load(
            "viewport/selection_mask",
            "viewport/selection_mask_vert.glsl",
            "viewport/selection_mask_frag.glsl");

        if (!selectionMaskShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the object selection mask shader",
                selectionMaskShaderResult.error());
        }
        append_startup_log("EditorViewport: selection mask shader loaded");

        const auto selectionMaskPassResult =
            selectionMaskPass_.create(shaderManager_);

        if (!selectionMaskPassResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the object selection mask pass",
                selectionMaskPassResult.error());
        }
        append_startup_log("EditorViewport: selection mask pass created");

        auto objectOutlineShaderResult = shaderManager_.load(
            "viewport/object_outline",
            "viewport/object_outline_vert.glsl",
            "viewport/object_outline_frag.glsl");

        if (!objectOutlineShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the object outline shader",
                objectOutlineShaderResult.error());
        }
        append_startup_log("EditorViewport: object outline shader loaded");

        const auto objectOutlinePassResult =
            objectOutlinePass_.create(shaderManager_);

        if (!objectOutlinePassResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the object outline pass",
                objectOutlinePassResult.error());
        }
        append_startup_log("EditorViewport: object outline pass created");

        framebufferWidth_ = framebufferWidth;
        framebufferHeight_ = framebufferHeight;

        if (framebufferWidth_ > 0 && framebufferHeight_ > 0) {
            const auto pickingBufferResult =
                pickingBuffer_.create(
                    framebufferWidth_,
                    framebufferHeight_);

            if (!pickingBufferResult) {
                shutdown();
                return graphics_failure(
                    ApplicationErrorCode::InitializationFailed,
                    "Failed to create the viewport picking buffer",
                    pickingBufferResult.error());
            }

            const auto selectionMaskResizeResult =
                selectionMaskPass_.resize(
                    framebufferWidth_,
                    framebufferHeight_);

            if (!selectionMaskResizeResult) {
                shutdown();
                return graphics_failure(
                    ApplicationErrorCode::InitializationFailed,
                    "Failed to create the object selection mask buffer",
                    selectionMaskResizeResult.error());
            }
        }

        const auto gridResult = gridRenderer_.create(
            meshUploader_,
            shaderManager_,
            graphics::GridRendererConfig{});

        if (!gridResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the viewport grid",
                gridResult.error());
        }
        append_startup_log("EditorViewport: grid renderer created");

        const auto axisResult = axisRenderer_.create(
            meshUploader_,
            shaderManager_,
            graphics::AxisRendererConfig{});

        if (!axisResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the viewport axes",
                axisResult.error());
        }
        append_startup_log("EditorViewport: axis renderer created");

        const auto gizmoResult = gizmoRenderer_.create(
            meshUploader_,
            shaderManager_,
            graphics::GizmoRendererConfig{});

        if (!gizmoResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the transform gizmo renderer",
                gizmoResult.error());
        }
        append_startup_log("EditorViewport: gizmo renderer created");

        initialized_ = true;
        append_startup_log("EditorViewport: initialized");
        return {};
    }

    void EditorViewport::shutdown()
    {
        renderQueue_.clear();
        pickingBuffer_.destroy();
        pickingRenderer_ = graphics::PickingRenderer{};
        objectOutlinePass_.destroy();
        selectionMaskPass_.destroy();
        activeDocumentId_ = {};
        documentShader_ = nullptr;
        lastPickingResult_ = {};
        framebufferWidth_ = 0;
        framebufferHeight_ = 0;
        lastPickingX_ = -1;
        lastPickingY_ = -1;
        pickingPassDirty_ = true;
        pickingQueryValid_ = false;

        axisRenderer_.destroy();
        gizmoRenderer_.destroy();
        topologyVertexRenderer_.destroy();
        topologyLineRenderer_.destroy();
        topologySurfaceRenderer_.destroy();
        gridRenderer_.destroy();
        meshCache_.clear();
        shaderManager_.clear();

        initialized_ = false;
    }

    bool EditorViewport::initialized() const noexcept
    {
        return initialized_;
    }

    void EditorViewport::resize(
        std::int32_t framebufferWidth,
        std::int32_t framebufferHeight)
    {
        if (framebufferWidth_ != framebufferWidth
            || framebufferHeight_ != framebufferHeight) {
            framebufferWidth_ = framebufferWidth;
            framebufferHeight_ = framebufferHeight;
            invalidate_picking();
            if (initialized_) {
                static_cast<void>(
                    selectionMaskPass_.resize(
                        framebufferWidth_,
                        framebufferHeight_));
            }
        }

        viewport_.resize(framebufferWidth, framebufferHeight);
    }

    void EditorViewport::orbit_camera(double deltaX, double deltaY)
    {
        if (deltaX == 0.0 && deltaY == 0.0) {
            return;
        }

        orbitRig_.orbit(
            static_cast<float>(-deltaX) * OrbitRadiansPerPixel,
            static_cast<float>(-deltaY) * OrbitRadiansPerPixel);
        viewOrientation_ = ViewOrientation::User;
        apply_orbit_rig_to_camera();
        invalidate_picking();
    }

    void EditorViewport::pan_camera(double deltaX, double deltaY)
    {
        if (deltaX == 0.0 && deltaY == 0.0) {
            return;
        }

        const float scale =
            viewport_.camera().projection().type()
                == graphics::ProjectionType::Orthographic
            ? viewport_.camera().projection().orthographic_height()
                / static_cast<float>(
                    std::max(viewport_.state().rect.height, 1))
            : orbitRig_.distance() * PerspectivePanDistancePerPixel;

        orbitRig_.pan(
            static_cast<float>(-deltaX) * scale,
            static_cast<float>(deltaY) * scale);
        apply_orbit_rig_to_camera();
        invalidate_picking();
    }

    void EditorViewport::zoom_camera(double scrollDelta)
    {
        if (scrollDelta == 0.0) {
            return;
        }

        graphics::Projection& projection =
            viewport_.camera().projection();

        if (projection.type() == graphics::ProjectionType::Orthographic) {
            const float zoomFactor = std::pow(
                0.90f,
                static_cast<float>(scrollDelta));
            projection.set_orthographic(
                projection.orthographic_height() * zoomFactor,
                projection.aspect_ratio(),
                projection.near_plane(),
                projection.far_plane());
            invalidate_picking();
            return;
        }

        const float step = std::max(
            orbitRig_.distance() * ZoomDistanceRatio,
            MinimumZoomStep);

        orbitRig_.zoom(
            static_cast<float>(-scrollDelta) * step);
        apply_orbit_rig_to_camera();
        invalidate_picking();
    }

    void EditorViewport::toggle_projection_mode()
    {
        set_projection_mode(
            viewport_.camera().projection().type()
                == graphics::ProjectionType::Perspective
            ? graphics::ProjectionType::Orthographic
            : graphics::ProjectionType::Perspective);
    }

    void EditorViewport::toggle_shading_mode() noexcept
    {
        set_shading_mode(
            toggle_viewport_shading_mode(displaySettings_.shadingMode));
    }

    void EditorViewport::toggle_face_orientation() noexcept
    {
        set_face_orientation_enabled(
            !displaySettings_.showFaceOrientation);
    }

    void EditorViewport::set_shading_mode(ViewportShadingMode mode) noexcept
    {
        displaySettings_.shadingMode = mode;
    }

    void EditorViewport::set_face_orientation_enabled(
        bool enabled) noexcept
    {
        displaySettings_.showFaceOrientation = enabled;
    }

    void EditorViewport::set_projection_mode(
        graphics::ProjectionType mode)
    {
        if (viewport_.camera().projection().type() == mode) {
            return;
        }

        preserve_framing_for_projection(mode);
        apply_orbit_rig_to_camera();
        invalidate_picking();
    }

    void EditorViewport::set_view_orientation(
        ViewOrientation orientation)
    {
        if (orientation == ViewOrientation::User) {
            viewOrientation_ = ViewOrientation::User;
            return;
        }

        orbitRig_.look(
            canonical_forward(orientation),
            canonical_up(orientation));
        viewOrientation_ = orientation;
        set_projection_mode(graphics::ProjectionType::Orthographic);
        apply_orbit_rig_to_camera();
        invalidate_picking();
    }

    graphics::ProjectionType
    EditorViewport::projection_mode() const noexcept
    {
        return viewport_.camera().projection().type();
    }

    ViewportShadingMode EditorViewport::shading_mode() const noexcept
    {
        return displaySettings_.shadingMode;
    }

    bool EditorViewport::face_orientation_enabled() const noexcept
    {
        return displaySettings_.showFaceOrientation;
    }

    ViewOrientation EditorViewport::view_orientation() const noexcept
    {
        return viewOrientation_;
    }

    float EditorViewport::world_units_per_pixel_at(
        const glm::vec3& worldPoint) const noexcept
    {
        const graphics::Camera& camera = viewport_.camera();
        const std::int32_t viewportHeight =
            std::max(viewport_.state().rect.height, 1);

        if (camera.projection().type()
            == graphics::ProjectionType::Orthographic) {
            return camera.projection().orthographic_height()
                / static_cast<float>(viewportHeight);
        }

        const float distance =
            std::max(
                glm::dot(worldPoint - camera.position(), camera.forward()),
                MinFramingDistance);
        return orthographic_height_for_distance(
            distance,
            camera.projection().vertical_fov_radians())
            / static_cast<float>(viewportHeight);
    }

    float EditorViewport::visual_scale_at(
        const glm::vec3& worldPoint) const noexcept
    {
        return world_units_per_pixel_at(worldPoint) * VisualScalePixels;
    }

    ApplicationResult<void> EditorViewport::render(
        DocumentSession& document)
    {
        if (!initialized_) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "EditorViewport must be initialized before render().");
        }

        editor::EditorSyncOptions syncOptions{};
        syncOptions.renderSceneOptions.sceneOptions.allowNullGpuMeshes = false;
        syncOptions.renderSceneOptions.sceneOptions.meshOptions.shader =
            documentShader_;
        syncOptions.renderSceneOptions.sceneOptions.meshRevisionResolver =
            [](const editor::MeshNode& node) {
                return static_cast<graphics::u64>(
                    node.mesh_revision());
            };

        const bool documentChanged =
            activeDocumentId_ != document.id();

        if (documentChanged) {
            meshCache_.clear();
            document.editor().mark_dirty(editor::EditorDirtyFlags::Render);
            activeDocumentId_ = document.id();
            invalidate_picking();
        }

        const editor::EditorDirtyFlags dirtyFlags =
            document.editor().dirty_flags();

        meshCache_.begin_frame();

        const auto syncResult =
            document.editor_sync().sync_cached_if_needed(
                document.editor(),
                meshCache_,
                meshUploader_,
                syncOptions);

        if (!syncResult) {
            return graphics_failure(
                ApplicationErrorCode::RuntimeFailure,
                "Failed to synchronize the document render scene",
                syncResult.error());
        }

        const editor::EditorDirtyFlags pickingInvalidationFlags =
            editor::EditorDirtyFlags::Scene |
            editor::EditorDirtyFlags::Mesh |
            editor::EditorDirtyFlags::Render |
            editor::EditorDirtyFlags::Picking;

        if (document.editor_sync().last_result().renderSceneSynced
            && editor::has_flag(
                dirtyFlags,
                pickingInvalidationFlags)) {
            invalidate_picking();
        }

        apply_orbit_rig_to_camera();
        gridRenderer_.update(viewport_.camera());
        viewport_.begin_frame();

        renderer_.set_view_matrix(viewport_.camera().view_matrix());
        renderer_.set_projection_matrix(
            viewport_.camera().projection_matrix());

        const graphics::RenderScene& scene =
            document.editor_sync().render_scene();

        graphics::GizmoDrawData gizmoData{};
        gizmoData.visible = false;

        if (editor::TransformTool* transformTool =
                active_transform_tool(document)) {
            editor::ToolContext toolContext =
                make_tool_context(document);
            transformTool->refresh_gizmo_state(toolContext);

            const editor::GizmoState& gizmoState =
                transformTool->gizmo_state();

            gizmoData.mode = to_visual_mode(gizmoState.mode);
            gizmoData.pivot = gizmoState.pivot;
            gizmoData.orientation = gizmoState.orientation;
            gizmoData.visualScale = visual_scale_at(gizmoData.pivot);
            gizmoData.viewDirection = viewport_.camera().forward();
            gizmoData.viewRight = viewport_.camera().right();
            gizmoData.viewUp = viewport_.camera().up();
            gizmoData.hovered =
                to_visual_selection(gizmoState.hovered);
            gizmoData.active =
                to_visual_selection(gizmoState.active);
            gizmoData.visible =
                gizmoState.visible
                && gizmoData.mode != graphics::GizmoVisualMode::None;
            gizmoData.enabled = gizmoState.enabled;
        }

        gizmoRenderer_.update(gizmoData);

        graphics::RenderScene gizmoScene{};
        gizmoScene.reserve(gizmoRenderer_.submitted_object_count());
        gizmoRenderer_.submit(gizmoScene);

        std::deque<graphics::GpuMesh> overlayMeshes;
        std::vector<graphics::RenderObject> overlayObjects;
        append_operation_preview(
            document,
            documentShader_,
            meshUploader_,
            overlayMeshes,
            overlayObjects);

        const ViewportShadingFrameConfig shadingConfig =
            viewport_shading_frame_config(displaySettings_);
        const bool drawGridBeforeSurfaceDepth =
            shadingConfig.surfaceDepthPrepass &&
            !shadingConfig.surfaceColorPass;

        editor::TopologyOverlayOptions topologyOptions{};
        const graphics::ScreenSpaceLineBatch topologyLines =
            displaySettings_.shadingMode == ViewportShadingMode::Wireframe
                ? editor::TopologyOverlayAdapter::build_visible_mesh_lines(
                    document.editor().scene(),
                    document.editor().selection(),
                    topologyOptions)
                : editor::TopologyOverlayAdapter::build_active_mesh_lines(
                    document.editor().scene(),
                    document.editor().selection(),
                    topologyOptions);
        const graphics::ScreenSpaceLineBatch occludedTopologyLines =
            make_occluded_lines(topologyLines);

        const graphics::PointMarkerBatch topologyVertices =
            editor::TopologyOverlayAdapter::build_active_mesh_vertex_markers(
                document.editor().scene(),
                document.editor().selection(),
                topologyOptions);

        const editor::TopologySurfaceOverlayBatches topologySurfaces =
            editor::TopologyOverlayAdapter::build_active_mesh_face_surfaces(
                document.editor().scene(),
                document.editor().selection(),
                topologyOptions);

        graphics::SurfaceOverlayBatch topologySurfaceBatch{};
        append_surface_overlay_batch(
            topologySurfaceBatch,
            topologySurfaces.hovered);
        append_surface_overlay_batch(
            topologySurfaceBatch,
            topologySurfaces.selected);

        if (topologySurfaceRenderer_.is_valid()) {
            const auto topologySurfaceUploadResult =
                topologySurfaceRenderer_.set_batch(topologySurfaceBatch);

            if (!topologySurfaceUploadResult) {
                return graphics_failure(
                    ApplicationErrorCode::RuntimeFailure,
                    "Failed to upload topology face surface overlays",
                    topologySurfaceUploadResult.error());
            }
        }

        if (topologyLineRenderer_.is_valid()) {
            const auto topologyUploadResult =
                topologyLineRenderer_.set_lines(topologyLines);

            if (!topologyUploadResult) {
                return graphics_failure(
                    ApplicationErrorCode::RuntimeFailure,
                    "Failed to upload topology overlay lines",
                    topologyUploadResult.error());
            }
        }

        if (!topologyVertices.empty() &&
            !topologyVertexRenderer_.is_valid()) {
            append_startup_log("EditorViewport: lazy point marker renderer create begin");
            graphics::PointMarkerRendererConfig pointMarkerConfig{};
            const auto createVertexRendererResult =
                topologyVertexRenderer_.create(
                    shaderManager_,
                    pointMarkerConfig);

            if (!createVertexRendererResult) {
                append_startup_log("EditorViewport: lazy point marker renderer create failed");
                topologyVertexRenderer_.destroy();
            }
            else {
                append_startup_log("EditorViewport: lazy point marker renderer created");
            }
        }

        if (topologyVertexRenderer_.is_valid()) {
            const auto topologyVertexUploadResult =
                topologyVertexRenderer_.set_markers(topologyVertices);

            if (!topologyVertexUploadResult) {
                return graphics_failure(
                    ApplicationErrorCode::RuntimeFailure,
                    "Failed to upload topology overlay vertex markers",
                    topologyVertexUploadResult.error());
            }
        }

        renderQueue_.clear();
        graphics::RenderQueue sceneSurfaceQueue{};
        graphics::RenderScene outlineScene{};
        outlineScene.reserve(scene.object_count());
        sceneSurfaceQueue.reserve(scene.object_count());

        renderQueue_.reserve(
            scene.object_count()
            + overlayObjects.size()
            + 1);
        if (!drawGridBeforeSurfaceDepth) {
            renderQueue_.add_object(gridRenderer_.render_object());
        }

        const editor::SceneNodeId previewTarget =
            operation_preview_target(document);

        for (const graphics::RenderObject& object : scene.objects()) {
            if (previewTarget.is_valid() &&
                object.id == static_cast<graphics::RenderObject::Id>(
                    previewTarget.value)) {
                continue;
            }

            renderQueue_.add_object(object);
            sceneSurfaceQueue.add_object(object);
            outlineScene.add_object(object);
        }

        sceneSurfaceQueue.sort();
        if (drawGridBeforeSurfaceDepth) {
            graphics::RenderQueue gridQueue{};
            gridQueue.reserve(1u);
            gridQueue.add_object(gridRenderer_.render_object());
            gridQueue.sort();
            renderer_.render(gridQueue);
        }

        if (shadingConfig.surfaceDepthPrepass) {
            renderer_.render_depth_only(sceneSurfaceQueue);
        }

        if (!shadingConfig.surfaceColorPass) {
            renderQueue_.clear();
            renderQueue_.reserve(overlayObjects.size() + 1u);
            if (!drawGridBeforeSurfaceDepth) {
                renderQueue_.add_object(gridRenderer_.render_object());
            }
        }

        for (const graphics::RenderObject& object : overlayObjects) {
            renderQueue_.add_object(object);
        }

        renderQueue_.sort();
        if (displaySettings_.showFaceOrientation) {
            graphics::ViewportPalette palette{};
            graphics::FaceOrientationDisplay orientationDisplay{};
            orientationDisplay.enabled = true;
            orientationDisplay.frontColor =
                palette.frontFaceOrientationColor;
            orientationDisplay.backColor =
                palette.backFaceOrientationColor;

            renderer_.set_face_orientation_display(orientationDisplay);
        }

        renderer_.render(renderQueue_);
        renderer_.set_face_orientation_display({});

        const auto selectionMaskResizeResult =
            selectionMaskPass_.resize(
                viewport_.state().rect.width,
                viewport_.state().rect.height);

        if (!selectionMaskResizeResult) {
            return graphics_failure(
                ApplicationErrorCode::RuntimeFailure,
                "Failed to resize the object selection mask buffer",
                selectionMaskResizeResult.error());
        }

        const graphics::ObjectHighlightBatch objectHighlights =
            editor::SelectionRenderAdapter::build_object_highlights(
                outlineScene,
                document.editor().selection());

        if (!objectHighlights.empty()) {
            graphics::ViewportPalette palette{};
            graphics::ObjectOutlinePassConfig outlineConfig{};
            outlineConfig.hoveredColor =
                palette.hoveredObjectOutlineColor;
            outlineConfig.hoveredWidthPixels =
                palette.hoveredObjectOutlineWidthPixels;
            outlineConfig.selectedColor =
                palette.selectedObjectOutlineColor;
            outlineConfig.selectedWidthPixels =
                palette.selectedObjectOutlineWidthPixels;
            objectOutlinePass_.set_config(outlineConfig);

            selectionMaskPass_.render(
                outlineScene,
                objectHighlights,
                viewport_.camera().view_matrix(),
                viewport_.camera().projection_matrix());

            if (const graphics::Texture* maskTexture =
                    selectionMaskPass_.mask_texture()) {
                objectOutlinePass_.render(
                    *maskTexture,
                    viewport_.state().rect);
            }
        }

        if (shadingConfig.topologySurfaceOverlays) {
            topologySurfaceRenderer_.render(
                viewport_.camera().view_projection_matrix());
        }

        if (shadingConfig.topologyOccludedEdges &&
            topologyLineRenderer_.is_valid()) {
            const auto occludedUploadResult =
                topologyLineRenderer_.set_lines(occludedTopologyLines);

            if (!occludedUploadResult) {
                return graphics_failure(
                    ApplicationErrorCode::RuntimeFailure,
                    "Failed to upload occluded topology overlay lines",
                    occludedUploadResult.error());
            }

            topologyLineRenderer_.render(
                viewport_.camera().view_projection_matrix(),
                viewport_.state().rect,
                graphics::DepthFunc::Greater);

            const auto visibleUploadResult =
                topologyLineRenderer_.set_lines(topologyLines);

            if (!visibleUploadResult) {
                return graphics_failure(
                    ApplicationErrorCode::RuntimeFailure,
                    "Failed to restore visible topology overlay lines",
                    visibleUploadResult.error());
            }
        }

        if (shadingConfig.topologyVisibleEdges) {
            topologyLineRenderer_.render(
                viewport_.camera().view_projection_matrix(),
                viewport_.state().rect,
                graphics::DepthFunc::LessEqual);
        }

        topologyVertexRenderer_.render(
            viewport_.camera().view_projection_matrix(),
            viewport_.state().rect);

        graphics::RenderQueue gizmoQueue;
        gizmoQueue.reserve(gizmoScene.object_count());

        for (const graphics::RenderObject& object : gizmoScene.objects()) {
            gizmoQueue.add_object(object);
        }

        gizmoQueue.sort();

        graphics::RendererSurfaceState occludedGizmoState{};
        occludedGizmoState.depthTest = true;
        occludedGizmoState.depthWrite = false;
        occludedGizmoState.depthFunc = graphics::DepthFunc::Greater;
        occludedGizmoState.blend = true;
        occludedGizmoState.sourceBlend = graphics::BlendFactor::SourceAlpha;
        occludedGizmoState.destinationBlend =
            graphics::BlendFactor::OneMinusSourceAlpha;
        occludedGizmoState.vertexAlphaMultiplier = 0.28f;
        occludedGizmoState.cullFace = false;

        renderer_.render_with_state(
            gizmoQueue,
            occludedGizmoState);

        graphics::RendererSurfaceState visibleGizmoState =
            occludedGizmoState;
        visibleGizmoState.depthFunc = graphics::DepthFunc::LessEqual;
        visibleGizmoState.vertexAlphaMultiplier = 1.0f;

        renderer_.render_with_state(
            gizmoQueue,
            visibleGizmoState);

        graphics::RenderQueue axisQueue;
        axisQueue.reserve(1);
        axisQueue.add_object(axisRenderer_.render_object());
        axisQueue.sort();

        renderer_.render_with_state(
            axisQueue,
            graphics::Renderer::foreground_overlay_state());

        return {};
    }

    ApplicationResult<ViewportPickingResult>
    EditorViewport::update_hover(
        DocumentSession& document,
        const InputVector2& cursor,
        std::int32_t logicalWidth,
        std::int32_t logicalHeight,
        bool focused,
        bool cameraCaptured,
        bool publishHover)
    {
        if (!initialized_) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "EditorViewport must be initialized before picking.");
        }

        if (!focused) {
            if (publishHover) {
                clear_hover(document, ViewportPickingStatus::FocusLost);
            }
            else {
                lastPickingResult_ = {};
                lastPickingResult_.status = ViewportPickingStatus::FocusLost;
                lastPickingX_ = -1;
                lastPickingY_ = -1;
                pickingQueryValid_ = false;
            }
            return lastPickingResult_;
        }

        if (cameraCaptured) {
            if (publishHover) {
                clear_hover(
                    document,
                    ViewportPickingStatus::CameraCapture);
            }
            else {
                lastPickingResult_ = {};
                lastPickingResult_.status = ViewportPickingStatus::CameraCapture;
                lastPickingX_ = -1;
                lastPickingY_ = -1;
                pickingQueryValid_ = false;
            }
            return lastPickingResult_;
        }

        if (framebufferWidth_ <= 0 || framebufferHeight_ <= 0) {
            if (publishHover) {
                clear_hover(
                    document,
                    ViewportPickingStatus::EmptyFramebuffer);
            }
            else {
                lastPickingResult_ = {};
                lastPickingResult_.status = ViewportPickingStatus::EmptyFramebuffer;
                lastPickingX_ = -1;
                lastPickingY_ = -1;
                pickingQueryValid_ = false;
            }
            return lastPickingResult_;
        }

        if (logicalWidth <= 0 || logicalHeight <= 0
            || cursor.x < 0.0 || cursor.y < 0.0
            || cursor.x >= static_cast<double>(logicalWidth)
            || cursor.y >= static_cast<double>(logicalHeight)) {
            if (publishHover) {
                clear_hover(
                    document,
                    ViewportPickingStatus::OutsideViewport);
            }
            else {
                lastPickingResult_ = {};
                lastPickingResult_.status = ViewportPickingStatus::OutsideViewport;
                lastPickingX_ = -1;
                lastPickingY_ = -1;
                pickingQueryValid_ = false;
            }
            return lastPickingResult_;
        }

        if (activeDocumentId_ != document.id()) {
            if (publishHover) {
                clear_hover(
                    document,
                    ViewportPickingStatus::Unavailable);
            }
            else {
                lastPickingResult_ = {};
                lastPickingResult_.status = ViewportPickingStatus::Unavailable;
                lastPickingX_ = -1;
                lastPickingY_ = -1;
                pickingQueryValid_ = false;
            }
            return lastPickingResult_;
        }

        const double framebufferScaleX =
            static_cast<double>(framebufferWidth_)
            / static_cast<double>(logicalWidth);
        const double framebufferScaleY =
            static_cast<double>(framebufferHeight_)
            / static_cast<double>(logicalHeight);

        const std::int32_t globalX = static_cast<std::int32_t>(
            std::floor(cursor.x * framebufferScaleX));
        const std::int32_t topY = static_cast<std::int32_t>(
            std::floor(cursor.y * framebufferScaleY));
        const std::int32_t globalY =
            framebufferHeight_ - 1 - topY;

        const graphics::ViewportRect& rect = viewport_.state().rect;
        if (globalX < rect.x || globalY < rect.y
            || globalX >= rect.x + rect.width
            || globalY >= rect.y + rect.height) {
            if (publishHover) {
                clear_hover(
                    document,
                    ViewportPickingStatus::OutsideViewport);
            }
            else {
                lastPickingResult_ = {};
                lastPickingResult_.status = ViewportPickingStatus::OutsideViewport;
                lastPickingX_ = -1;
                lastPickingY_ = -1;
                pickingQueryValid_ = false;
            }
            return lastPickingResult_;
        }

        const std::int32_t localX = globalX - rect.x;
        const std::int32_t localY = globalY - rect.y;

        const auto bufferResult = ensure_picking_buffer();
        if (!bufferResult) {
            return bufferResult.error();
        }

        bool bufferRendered = false;
        if (pickingPassDirty_) {
            const graphics::Camera& camera = viewport_.camera();
            pickingRenderer_.set_view_matrix(camera.view_matrix());
            pickingRenderer_.set_projection_matrix(
                camera.projection_matrix());
            pickingRenderer_.render(
                pickingBuffer_,
                document.editor_sync().render_scene());

            pickingPassDirty_ = false;
            pickingQueryValid_ = false;
            bufferRendered = true;
        }

        if (pickingQueryValid_
            && localX == lastPickingX_
            && localY == lastPickingY_) {
            ViewportPickingResult cached = lastPickingResult_;
            cached.bufferRendered = bufferRendered;
            cached.pixelRead = false;

            if (publishHover) {
                set_hover(document, cached.sceneNodeId);
            }

            return cached;
        }

        const graphics::PickingId pickingId =
            pickingBuffer_.read_id(localX, localY);
        const editor::SceneNodeId sceneNodeId =
            document.editor_sync().picking_sync().scene_node_id(
                pickingId);

        lastPickingResult_ = {};
        lastPickingResult_.status =
            pickingId.is_valid() && sceneNodeId.is_valid()
            ? ViewportPickingStatus::Hit
            : ViewportPickingStatus::Background;
        lastPickingResult_.pickingId = pickingId;
        lastPickingResult_.sceneNodeId = sceneNodeId;
        lastPickingResult_.framebufferX = localX;
        lastPickingResult_.framebufferY = localY;
        lastPickingResult_.bufferRendered = bufferRendered;
        lastPickingResult_.pixelRead = true;

        lastPickingX_ = localX;
        lastPickingY_ = localY;
        pickingQueryValid_ = true;

        if (publishHover) {
            set_hover(document, sceneNodeId);
        }

        return lastPickingResult_;
    }

    const ViewportPickingResult&
    EditorViewport::last_picking_result() const noexcept
    {
        return lastPickingResult_;
    }

    graphics::Viewport& EditorViewport::viewport() noexcept
    {
        invalidate_picking();
        return viewport_;
    }

    const graphics::Viewport& EditorViewport::viewport() const noexcept
    {
        return viewport_;
    }

    graphics::OrbitCameraRig& EditorViewport::orbit_rig() noexcept
    {
        invalidate_picking();
        return orbitRig_;
    }

    const graphics::OrbitCameraRig&
    EditorViewport::orbit_rig() const noexcept
    {
        return orbitRig_;
    }

    const graphics::Renderer& EditorViewport::renderer() const noexcept
    {
        return renderer_;
    }

    float EditorViewport::aspect_ratio() const noexcept
    {
        return viewport_.state().aspectRatio;
    }

    void EditorViewport::apply_orbit_rig_to_camera()
    {
        orbitRig_.apply(viewport_.camera());
    }

    void EditorViewport::preserve_framing_for_projection(
        graphics::ProjectionType mode)
    {
        graphics::Projection& projection =
            viewport_.camera().projection();

        if (mode == graphics::ProjectionType::Orthographic) {
            projection.set_orthographic(
                orthographic_height_for_distance(
                    orbitRig_.distance(),
                    projection.vertical_fov_radians()),
                projection.aspect_ratio(),
                projection.near_plane(),
                projection.far_plane());
            return;
        }

        orbitRig_.set_distance(
            perspective_distance_for_height(
                projection.orthographic_height(),
                projection.vertical_fov_radians()));
        projection.set_perspective(
            projection.vertical_fov_radians(),
            projection.aspect_ratio(),
            projection.near_plane(),
            projection.far_plane());
    }

    ApplicationResult<void> EditorViewport::ensure_picking_buffer()
    {
        if (pickingBuffer_.is_valid()
            && pickingBuffer_.width() == framebufferWidth_
            && pickingBuffer_.height() == framebufferHeight_) {
            return {};
        }

        graphics::GraphicsResult<void> result =
            pickingBuffer_.is_valid()
            ? pickingBuffer_.resize(
                framebufferWidth_,
                framebufferHeight_)
            : pickingBuffer_.create(
                framebufferWidth_,
                framebufferHeight_);

        if (!result) {
            return graphics_failure(
                ApplicationErrorCode::RuntimeFailure,
                "Failed to resize the viewport picking buffer",
                result.error());
        }

        invalidate_picking();
        return {};
    }

    void EditorViewport::invalidate_picking() noexcept
    {
        pickingPassDirty_ = true;
        pickingQueryValid_ = false;
    }

    void EditorViewport::clear_hover(
        DocumentSession& document,
        ViewportPickingStatus status)
    {
        set_hover(document, editor::SceneNodeId{});
        lastPickingResult_ = {};
        lastPickingResult_.status = status;
        lastPickingX_ = -1;
        lastPickingY_ = -1;
        pickingQueryValid_ = false;
    }

    void EditorViewport::set_hover(
        DocumentSession& document,
        editor::SceneNodeId nodeId)
    {
        const editor::Editor& readOnlyEditor = document.editor();
        if (readOnlyEditor.selection().objects().hovered() == nodeId) {
            return;
        }

        const bool changed = document.editor()
            .selection_controller()
            .set_hovered_object(nodeId);

        if (changed) {
            document.editor().mark_dirty(
                editor::EditorDirtyFlags::Selection |
                editor::EditorDirtyFlags::Render);
        }
    }

} // namespace locus::application
