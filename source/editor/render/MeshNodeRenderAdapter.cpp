/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/MeshNodeRenderAdapter.h"

#include "editor/render/RenderMeshUploadAdapter.h"
#include "editor/scene/MeshNode.h"
#include "graphics/common/GraphicsError.h"
#include "kernel/geometry/render/MeshTriangulator.h"
#include "kernel/geometry/render/RenderMesh.h"

namespace locus::editor {

    graphics::MeshUploadData MeshNodeRenderAdapter::build_upload_data(
        const MeshNode& node,
        const MeshNodeRenderOptions& options,
        MeshNodeRenderResult* result
    ) {
        if (result) {
            *result = {};
            result->nodeId = node.id();
        }

        const kernel::geometry::RenderMesh renderMesh =
            kernel::geometry::MeshTriangulator::triangulate(node.mesh());

        RenderMeshUploadResult uploadResult{};
        graphics::MeshUploadData uploadData =
            RenderMeshUploadAdapter::build_triangle_upload_data(
                renderMesh,
                options.uploadOptions,
                &uploadResult
            );

        if (result) {
            result->uploadResult = uploadResult;
            result->hasUploadData = !uploadData.is_empty();
        }

        if (uploadData.is_empty()) {
            if (result) {
                result->skipped = true;
            }

            if (options.reportEmptyMeshes) {
                set_message(result, "Mesh node produced empty triangle upload data.");
            }

            return uploadData;
        }

        set_message(result, "Mesh node upload data built successfully.");
        return uploadData;
    }

    graphics::RenderObject MeshNodeRenderAdapter::build_render_object(
        const MeshNode& node,
        const graphics::GpuMesh* gpuMesh,
        const MeshNodeRenderOptions& options
    ) {
        graphics::RenderObject object{};

        object.id = static_cast<graphics::RenderObject::Id>(node.id().value);
        object.name = node.metadata().name;
        object.mesh = gpuMesh;
        object.shader = options.shader;
        object.material = options.material;
        object.transform = build_render_transform(node);
        object.visibility = build_render_visibility(node);
        object.layer = options.layer;
        object.selected = options.selected;
        object.hovered = options.hovered;
        object.wireframe = options.wireframe;

        return object;
    }

    graphics::GraphicsResult<graphics::RenderObject> MeshNodeRenderAdapter::build_cached_render_object(
        const MeshNode& node,
        graphics::u64 meshRevision,
        graphics::MeshRenderCache& cache,
        const graphics::MeshUploader& uploader,
        const MeshNodeRenderOptions& options,
        MeshNodeRenderResult* result
    ) {
        if (result) {
            *result = {};
            result->nodeId = node.id();
        }

        if (node.id().is_invalid()) {
            set_message(result, "Cannot build render object for mesh node with invalid id.");
            return graphics::GraphicsError::make(
                graphics::GraphicsErrorCode::InvalidArgument,
                "Cannot build render object for mesh node with invalid id."
            );
        }

        const graphics::MeshRenderCacheKey cacheKey = build_cache_key(node, meshRevision);

        if (result) {
            result->cacheKey = cacheKey;
        }

        if (!cacheKey.is_valid()) {
            set_message(result, "Cannot build render object with invalid mesh cache key.");
            return graphics::GraphicsError::make(
                graphics::GraphicsErrorCode::InvalidArgument,
                "Cannot build render object with invalid mesh cache key."
            );
        }

        graphics::MeshUploadData uploadData = build_upload_data(node, options, result);

        if (uploadData.is_empty()) {
            set_message(result, "Cannot upload empty mesh node render data.");
            return graphics::GraphicsError::make(
                graphics::GraphicsErrorCode::InvalidArgument,
                "Cannot upload empty mesh node render data."
            );
        }

        auto meshResult = cache.get_or_upload(cacheKey, uploadData, uploader);

        if (!meshResult) {
            set_message(result, meshResult.error().message);
            return meshResult.error();
        }

        const graphics::GpuMesh* gpuMesh = meshResult.value();

        if (gpuMesh == nullptr) {
            set_message(result, "MeshRenderCache returned a null GPU mesh.");
            return graphics::GraphicsError::make(
                graphics::GraphicsErrorCode::ResourceNotFound,
                "MeshRenderCache returned a null GPU mesh."
            );
        }

        graphics::RenderObject object = build_render_object(node, gpuMesh, options);

        if (result) {
            result->hasGpuMesh = true;
            result->hasRenderObject = true;
        }

        set_message(result, "Mesh node render object built successfully.");
        return object;
    }

    graphics::MeshRenderCacheKey MeshNodeRenderAdapter::build_cache_key(
        const MeshNode& node,
        graphics::u64 meshRevision
    ) {
        graphics::MeshRenderCacheKey key{};
        key.ownerId = static_cast<graphics::u64>(node.id().value);
        key.revision = meshRevision;
        return key;
    }

    graphics::RenderTransform MeshNodeRenderAdapter::build_render_transform(const MeshNode& node)
    {
        graphics::RenderTransform transform{};
        transform.position = node.transform().position();
        transform.rotation = node.transform().rotation();
        transform.scale = node.transform().scale();
        return transform;
    }

    graphics::RenderVisibility MeshNodeRenderAdapter::build_render_visibility(const MeshNode& node)
    {
        graphics::RenderVisibility visibility{};
        visibility.visible = node.is_visible();
        visibility.selectable = node.is_selectable();
        visibility.castsShadow = false;
        visibility.receivesShadow = false;
        return visibility;
    }

    void MeshNodeRenderAdapter::set_message(MeshNodeRenderResult* result, std::string message)
    {
        if (!result) {
            return;
        }

        result->message = std::move(message);
    }

} // namespace locus::editor