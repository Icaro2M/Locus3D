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
#include "editor/scene/MeshNode.h"
#include "editor/selection/SelectionState.h"
#include "editor/sync/EditorSync.h"
#include "editor/tools/transform/TransformTool.h"
#include "graphics/common/GraphicsError.h"
#include "graphics/primitives/PrimitiveMeshConverter.h"
#include "graphics/scene/RenderObject.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace locus::application {

    namespace {

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

        [[nodiscard]] const editor::TransformTool* active_transform_tool(
            const DocumentSession& document)
        {
            const editor::ITool* tool =
                document.tool_manager().active_tool();

            return dynamic_cast<const editor::TransformTool*>(tool);
        }

        void append_mesh_component_overlays(
            const editor::Editor& editor,
            const graphics::Shader* shader,
            const graphics::MeshUploader& uploader,
            std::vector<graphics::GpuMesh>& meshes,
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

            editor::OverlayGeometry overlay =
                editor::OverlayRenderAdapter::build_mesh_overlay(
                    *meshNode,
                    editor.selection().mesh());

            if (!overlay.has_geometry()) {
                return;
            }

            meshes.reserve(meshes.size() + overlay.groups.size());
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
        orbitRig_.apply(viewport_.camera());

        shaderManager_.set_shader_root(shaderRoot);

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

        auto documentShaderResult = shaderManager_.load(
            "viewport/document",
            "debug/debug_vert.glsl",
            "debug/debug_frag.glsl");

        if (!documentShaderResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to load the document scene shader",
                documentShaderResult.error());
        }

        documentShader_ = documentShaderResult.value();

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

        const auto pickingRendererResult =
            pickingRenderer_.create(shaderManager_);

        if (!pickingRendererResult) {
            shutdown();
            return graphics_failure(
                ApplicationErrorCode::InitializationFailed,
                "Failed to create the viewport picking renderer",
                pickingRendererResult.error());
        }

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

        initialized_ = true;
        return {};
    }

    void EditorViewport::shutdown()
    {
        renderQueue_.clear();
        pickingBuffer_.destroy();
        pickingRenderer_ = graphics::PickingRenderer{};
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
        }

        viewport_.resize(framebufferWidth, framebufferHeight);
    }

    void EditorViewport::orbit_camera(double deltaX, double deltaY)
    {
        if (deltaX == 0.0 && deltaY == 0.0) {
            return;
        }

        constexpr float OrbitRadiansPerPixel = 0.005f;
        orbitRig_.orbit(
            static_cast<float>(-deltaX) * OrbitRadiansPerPixel,
            static_cast<float>(-deltaY) * OrbitRadiansPerPixel);
        invalidate_picking();
    }

    void EditorViewport::pan_camera(double deltaX, double deltaY)
    {
        if (deltaX == 0.0 && deltaY == 0.0) {
            return;
        }

        constexpr float PanDistancePerPixel = 0.0015f;
        const float scale =
            orbitRig_.distance() * PanDistancePerPixel;

        orbitRig_.pan(
            static_cast<float>(-deltaX) * scale,
            static_cast<float>(deltaY) * scale);
        invalidate_picking();
    }

    void EditorViewport::zoom_camera(double scrollDelta)
    {
        if (scrollDelta == 0.0) {
            return;
        }

        constexpr float ZoomDistanceRatio = 0.1f;
        constexpr float MinimumZoomStep = 0.05f;
        const float step = std::max(
            orbitRig_.distance() * ZoomDistanceRatio,
            MinimumZoomStep);

        orbitRig_.zoom(
            static_cast<float>(-scrollDelta) * step);
        invalidate_picking();
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

        orbitRig_.apply(viewport_.camera());
        gridRenderer_.update(viewport_.camera());
        viewport_.begin_frame();

        renderer_.set_view_matrix(viewport_.camera().view_matrix());
        renderer_.set_projection_matrix(
            viewport_.camera().projection_matrix());

        const graphics::RenderScene& scene =
            document.editor_sync().render_scene();

        graphics::GizmoDrawData gizmoData{};
        gizmoData.visible = false;

        if (const editor::TransformTool* transformTool =
                active_transform_tool(document)) {
            const editor::GizmoState& gizmoState =
                transformTool->gizmo_state();

            const bool hasSelection =
                !document.editor().selection().objects().empty();

            gizmoData.mode = to_visual_mode(gizmoState.mode);
            gizmoData.pivot = gizmoState.pivot;
            gizmoData.orientation = gizmoState.orientation;
            gizmoData.visualScale = gizmoState.visualScale;
            gizmoData.viewDirection = viewport_.camera().forward();
            gizmoData.viewRight = viewport_.camera().right();
            gizmoData.viewUp = viewport_.camera().up();
            gizmoData.hovered =
                to_visual_selection(gizmoState.hovered);
            gizmoData.active =
                to_visual_selection(gizmoState.active);
            gizmoData.visible =
                hasSelection
                && gizmoState.visible
                && gizmoData.mode != graphics::GizmoVisualMode::None;
            gizmoData.enabled = gizmoState.enabled;
        }

        gizmoRenderer_.update(gizmoData);

        graphics::RenderScene gizmoScene{};
        gizmoScene.reserve(gizmoRenderer_.submitted_object_count());
        gizmoRenderer_.submit(gizmoScene);

        std::vector<graphics::GpuMesh> overlayMeshes;
        std::vector<graphics::RenderObject> overlayObjects;
        append_mesh_component_overlays(
            document.editor(),
            documentShader_,
            meshUploader_,
            overlayMeshes,
            overlayObjects);

        renderQueue_.clear();
        renderQueue_.reserve(
            scene.object_count()
            + overlayObjects.size()
            + 2
            + gizmoScene.object_count());
        renderQueue_.add_object(gridRenderer_.render_object());

        for (const graphics::RenderObject& object : scene.objects()) {
            renderQueue_.add_object(object);
        }

        for (const graphics::RenderObject& object : overlayObjects) {
            renderQueue_.add_object(object);
        }

        for (const graphics::RenderObject& object : gizmoScene.objects()) {
            renderQueue_.add_object(object);
        }

        renderQueue_.add_object(axisRenderer_.render_object());
        renderQueue_.sort();
        renderer_.render(renderQueue_);

        return {};
    }

    ApplicationResult<ViewportPickingResult>
    EditorViewport::update_hover(
        DocumentSession& document,
        const InputVector2& cursor,
        std::int32_t logicalWidth,
        std::int32_t logicalHeight,
        bool focused,
        bool cameraCaptured)
    {
        if (!initialized_) {
            return ApplicationError::make(
                ApplicationErrorCode::InvalidState,
                "EditorViewport must be initialized before picking.");
        }

        if (!focused) {
            clear_hover(document, ViewportPickingStatus::FocusLost);
            return lastPickingResult_;
        }

        if (cameraCaptured) {
            clear_hover(
                document,
                ViewportPickingStatus::CameraCapture);
            return lastPickingResult_;
        }

        if (framebufferWidth_ <= 0 || framebufferHeight_ <= 0) {
            clear_hover(
                document,
                ViewportPickingStatus::EmptyFramebuffer);
            return lastPickingResult_;
        }

        if (logicalWidth <= 0 || logicalHeight <= 0
            || cursor.x < 0.0 || cursor.y < 0.0
            || cursor.x >= static_cast<double>(logicalWidth)
            || cursor.y >= static_cast<double>(logicalHeight)) {
            clear_hover(
                document,
                ViewportPickingStatus::OutsideViewport);
            return lastPickingResult_;
        }

        if (activeDocumentId_ != document.id()) {
            clear_hover(
                document,
                ViewportPickingStatus::Unavailable);
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
            clear_hover(
                document,
                ViewportPickingStatus::OutsideViewport);
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
