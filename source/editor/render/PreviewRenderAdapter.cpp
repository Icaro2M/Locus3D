/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/render/PreviewRenderAdapter.h"

#include "editor/render/RenderMeshUploadAdapter.h"
#include "editor/scene/MeshNode.h"

#include <utility>

namespace locus::editor {

    graphics::MeshUploadData
        PreviewRenderAdapter::build_solid_upload_data(
            const kernel::modeling::OperationPreview& preview,
            const PreviewRenderOptions& options,
            PreviewRenderResult* result)
    {
        if (result) {
            *result = {};
            result->status = preview.status();
        }

        if (!options.includeSolid) {
            if (result) {
                result->skipped = true;
            }

            set_message(
                result,
                "Solid preview conversion is disabled.");

            return {};
        }

        if (!validate_preview(preview, result)) {
            return {};
        }

        RenderMeshUploadResult uploadResult{};

        graphics::MeshUploadData uploadData =
            RenderMeshUploadAdapter::build_triangle_upload_data(
                preview.mesh().solid_mesh(),
                options.solidUploadOptions,
                &uploadResult);

        if (result) {
            result->solidUploadResult = uploadResult;
            result->hasSolidUploadData = !uploadData.is_empty();
        }

        if (uploadData.is_empty()) {
            if (options.reportEmptyGeometry) {
                set_message(
                    result,
                    "Operation preview produced empty solid upload data.");
            }

            return uploadData;
        }

        set_message(
            result,
            "Solid preview upload data built successfully.");

        return uploadData;
    }

    graphics::MeshUploadData
        PreviewRenderAdapter::build_wire_upload_data(
            const kernel::modeling::OperationPreview& preview,
            const PreviewRenderOptions& options,
            PreviewRenderResult* result)
    {
        if (result) {
            *result = {};
            result->status = preview.status();
        }

        if (!options.includeWire) {
            if (result) {
                result->skipped = true;
            }

            set_message(
                result,
                "Wire preview conversion is disabled.");

            return {};
        }

        if (!validate_preview(preview, result)) {
            return {};
        }

        RenderMeshUploadResult uploadResult{};

        graphics::MeshUploadData uploadData =
            RenderMeshUploadAdapter::build_line_upload_data(
                preview.mesh().wire_mesh(),
                options.wireUploadOptions,
                &uploadResult);

        if (result) {
            result->wireUploadResult = uploadResult;
            result->hasWireUploadData = !uploadData.is_empty();
        }

        if (uploadData.is_empty()) {
            if (options.reportEmptyGeometry) {
                set_message(
                    result,
                    "Operation preview produced empty wire upload data.");
            }

            return uploadData;
        }

        set_message(
            result,
            "Wire preview upload data built successfully.");

        return uploadData;
    }

    PreviewRenderObjects
        PreviewRenderAdapter::build_render_objects(
            const MeshNode& sourceNode,
            const kernel::modeling::OperationPreview& preview,
            const graphics::GpuMesh* solidMesh,
            const graphics::GpuMesh* wireMesh,
            const PreviewRenderOptions& options,
            PreviewRenderResult* result)
    {
        PreviewRenderObjects objects{};

        if (result) {
            *result = {};
            result->nodeId = sourceNode.id();
            result->status = preview.status();
        }

        if (sourceNode.id().is_invalid()) {
            set_message(
                result,
                "Cannot build preview render objects for a node with invalid id.");

            return objects;
        }

        if (!validate_preview(preview, result)) {
            return objects;
        }

        const graphics::RenderTransform transform =
            build_render_transform(sourceNode);

        const graphics::RenderVisibility visibility =
            build_render_visibility(sourceNode);

        if (options.includeSolid && solidMesh != nullptr) {
            graphics::RenderObject solidObject{};

            solidObject.id = options.solidObjectId;
            solidObject.name =
                sourceNode.metadata().name + " Preview Solid";
            solidObject.mesh = solidMesh;
            solidObject.shader = options.solidShader;
            solidObject.material = options.solidMaterial;
            solidObject.transform = transform;
            solidObject.visibility = visibility;
            solidObject.layer = options.solidLayer;
            solidObject.selected = false;
            solidObject.hovered = false;
            solidObject.wireframe = false;

            objects.solid = std::move(solidObject);
            objects.hasSolid = true;

            if (result) {
                result->hasSolidObject = true;
            }
        }

        if (options.includeWire && wireMesh != nullptr) {
            graphics::RenderObject wireObject{};

            wireObject.id = options.wireObjectId;
            wireObject.name =
                sourceNode.metadata().name + " Preview Wire";
            wireObject.mesh = wireMesh;
            wireObject.shader = options.wireShader;
            wireObject.material = options.wireMaterial;
            wireObject.transform = transform;
            wireObject.visibility = visibility;
            wireObject.layer = options.wireLayer;
            wireObject.selected = false;
            wireObject.hovered = false;

            /*
             * The supplied GPU mesh already uses line topology. Enabling the
             * RenderObject wireframe flag would request triangle rasterization
             * in wireframe mode and is therefore unnecessary here.
             */
            wireObject.wireframe = false;

            objects.wire = std::move(wireObject);
            objects.hasWire = true;

            if (result) {
                result->hasWireObject = true;
            }
        }

        if (objects.empty()) {
            if (result) {
                result->skipped = true;
            }

            set_message(
                result,
                "No resolved GPU meshes were provided for the operation preview.");

            return objects;
        }

        set_message(
            result,
            "Operation preview render objects built successfully.");

        return objects;
    }

    bool PreviewRenderAdapter::validate_preview(
        const kernel::modeling::OperationPreview& preview,
        PreviewRenderResult* result)
    {
        if (result) {
            result->status = preview.status();
            result->previewReady = preview.is_ready();
        }

        if (preview.is_failure()) {
            set_message(
                result,
                preview.message().empty()
                ? "Cannot convert a failed operation preview."
                : preview.message());

            return false;
        }

        if (preview.is_invalidated()) {
            set_message(
                result,
                preview.message().empty()
                ? "Cannot convert an invalidated operation preview."
                : preview.message());

            return false;
        }

        if (preview.is_empty()) {
            if (result) {
                result->skipped = true;
            }

            set_message(
                result,
                preview.message().empty()
                ? "Operation preview contains no displayable geometry."
                : preview.message());

            return false;
        }

        if (!preview.is_ready()) {
            set_message(
                result,
                "Operation preview is not ready for display.");

            return false;
        }

        if (!preview.mesh().valid()) {
            set_message(
                result,
                preview.mesh().message().empty()
                ? "Operation preview contains an invalid mesh payload."
                : preview.mesh().message());

            return false;
        }

        if (result) {
            result->previewReady = true;
        }

        return true;
    }

    graphics::RenderTransform
        PreviewRenderAdapter::build_render_transform(
            const MeshNode& node)
    {
        graphics::RenderTransform transform{};

        transform.position = node.transform().position();
        transform.rotation = node.transform().rotation();
        transform.scale = node.transform().scale();

        return transform;
    }

    graphics::RenderVisibility
        PreviewRenderAdapter::build_render_visibility(
            const MeshNode& node)
    {
        graphics::RenderVisibility visibility{};

        visibility.visible = node.is_visible();

        /*
         * Preview geometry must not compete with authoritative document
         * geometry in the picking pass.
         */
        visibility.selectable = false;
        visibility.castsShadow = false;
        visibility.receivesShadow = false;

        return visibility;
    }

    void PreviewRenderAdapter::set_message(
        PreviewRenderResult* result,
        std::string message)
    {
        if (!result) {
            return;
        }

        result->message = std::move(message);
    }

} // namespace locus::editor