/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/viewport/EditorViewport.h"

#include "application/document/DocumentSession.h"
#include "editor/EditorTypes.h"
#include "editor/sync/EditorSync.h"
#include "graphics/common/GraphicsError.h"
#include "graphics/scene/RenderObject.h"

#include <string>
#include <utility>

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

        initialized_ = true;
        return {};
    }

    void EditorViewport::shutdown()
    {
        renderQueue_.clear();
        activeDocumentId_ = {};
        documentShader_ = nullptr;

        axisRenderer_.destroy();
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
        viewport_.resize(framebufferWidth, framebufferHeight);
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

        if (activeDocumentId_ != document.id()) {
            document.editor().mark_dirty(editor::EditorDirtyFlags::Render);
            activeDocumentId_ = document.id();
        }

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

        orbitRig_.apply(viewport_.camera());
        gridRenderer_.update(viewport_.camera());
        viewport_.begin_frame();

        renderer_.set_view_matrix(viewport_.camera().view_matrix());
        renderer_.set_projection_matrix(
            viewport_.camera().projection_matrix());

        const graphics::RenderScene& scene =
            document.editor_sync().render_scene();

        renderQueue_.clear();
        renderQueue_.reserve(scene.object_count() + 2);
        renderQueue_.add_object(gridRenderer_.render_object());

        for (const graphics::RenderObject& object : scene.objects()) {
            renderQueue_.add_object(object);
        }

        renderQueue_.add_object(axisRenderer_.render_object());
        renderQueue_.sort();
        renderer_.render(renderQueue_);

        return {};
    }

    graphics::Viewport& EditorViewport::viewport() noexcept
    {
        return viewport_;
    }

    const graphics::Viewport& EditorViewport::viewport() const noexcept
    {
        return viewport_;
    }

    graphics::OrbitCameraRig& EditorViewport::orbit_rig() noexcept
    {
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

} // namespace locus::application
