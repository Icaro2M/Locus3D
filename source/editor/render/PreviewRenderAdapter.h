/*
* SPDX-FileCopyrightText: 2026 Icaro2M
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "editor/render/RenderAdapterTypes.h"
#include "editor/scene/SceneNodeId.h"
#include "graphics/mesh/MeshUploadData.h"
#include "graphics/scene/RenderLayer.h"
#include "graphics/scene/RenderObject.h"
#include "kernel/modeling/preview/OperationPreview.h"

#include <string>

namespace locus::editor {

    class MeshNode;

    /**
     * @brief Options used when converting operation previews to render data.
     */
    struct PreviewRenderOptions {
        /**
         * @brief Upload conversion options used for the solid preview mesh.
         */
        RenderMeshUploadOptions solidUploadOptions{};

        /**
         * @brief Upload conversion options used for the wire preview mesh.
         */
        RenderMeshUploadOptions wireUploadOptions{};

        /**
         * @brief Logical layer assigned to the solid preview object.
         */
        graphics::RenderLayer solidLayer =
            graphics::RenderLayer::Preview;

        /**
         * @brief Logical layer assigned to the wire preview object.
         */
        graphics::RenderLayer wireLayer =
            graphics::RenderLayer::Preview;

        /**
         * @brief Fallback shader used by the solid preview object.
         */
        const graphics::Shader* solidShader = nullptr;

        /**
         * @brief Fallback shader used by the wire preview object.
         */
        const graphics::Shader* wireShader = nullptr;

        /**
         * @brief Optional material used by the solid preview object.
         */
        const graphics::VisualMaterialInstance* solidMaterial = nullptr;

        /**
         * @brief Optional material used by the wire preview object.
         */
        const graphics::VisualMaterialInstance* wireMaterial = nullptr;

        /**
         * @brief Identifier assigned to the generated solid render object.
         *
         * The synchronization layer is responsible for providing an identifier
         * that does not collide with authoritative scene objects.
         */
        graphics::RenderObject::Id solidObjectId = 0;

        /**
         * @brief Identifier assigned to the generated wire render object.
         *
         * The synchronization layer is responsible for providing an identifier
         * that does not collide with authoritative scene objects.
         */
        graphics::RenderObject::Id wireObjectId = 0;

        /**
         * @brief True when solid preview geometry should be converted.
         */
        bool includeSolid = true;

        /**
         * @brief True when wire preview geometry should be converted.
         */
        bool includeWire = true;

        /**
         * @brief True when empty preview parts should produce diagnostics.
         */
        bool reportEmptyGeometry = true;
    };

    /**
     * @brief Diagnostics produced while converting an operation preview.
     */
    struct PreviewRenderResult {
        /**
         * @brief Source editor node identifier.
         */
        SceneNodeId nodeId{};

        /**
         * @brief Current kernel preview status.
         */
        kernel::modeling::OperationPreviewStatus status =
            kernel::modeling::OperationPreviewStatus::Empty;

        /**
         * @brief Statistics produced for the solid preview mesh.
         */
        RenderMeshUploadResult solidUploadResult{};

        /**
         * @brief Statistics produced for the wire preview mesh.
         */
        RenderMeshUploadResult wireUploadResult{};

        /**
         * @brief True when the source preview is ready for display.
         */
        bool previewReady = false;

        /**
         * @brief True when solid upload data was generated.
         */
        bool hasSolidUploadData = false;

        /**
         * @brief True when wire upload data was generated.
         */
        bool hasWireUploadData = false;

        /**
         * @brief True when a solid render object was generated.
         */
        bool hasSolidObject = false;

        /**
         * @brief True when a wire render object was generated.
         */
        bool hasWireObject = false;

        /**
         * @brief True when conversion was intentionally skipped.
         */
        bool skipped = false;

        /**
         * @brief Human-readable conversion diagnostic.
         */
        std::string message;
    };

    /**
     * @brief Pair of render objects generated for an operation preview.
     */
    struct PreviewRenderObjects {
        /**
         * @brief Solid shaded preview object.
         */
        graphics::RenderObject solid{};

        /**
         * @brief Wire overlay preview object.
         */
        graphics::RenderObject wire{};

        /**
         * @brief True when the solid object is available.
         */
        bool hasSolid = false;

        /**
         * @brief True when the wire object is available.
         */
        bool hasWire = false;

        /**
         * @brief Checks whether no render objects were generated.
         *
         * @return True when neither solid nor wire objects are available.
         */
        [[nodiscard]]
        bool empty() const
        {
            return !hasSolid && !hasWire;
        }
    };

    /**
     * @brief Converts kernel operation previews into graphics render payloads.
     *
     * PreviewRenderAdapter performs integration only. It does not execute kernel
     * operations, own GPU resources, mutate the editor scene, or insert objects
     * into a graphics render scene.
     */
    class PreviewRenderAdapter {
    public:
        /**
         * @brief Builds triangle upload data from the solid preview mesh.
         *
         * @param preview Source operation preview.
         * @param options Conversion options.
         * @param result Optional diagnostic output.
         * @return CPU-side triangle upload data.
         */
        [[nodiscard]]
        static graphics::MeshUploadData build_solid_upload_data(
            const kernel::modeling::OperationPreview& preview,
            const PreviewRenderOptions& options = {},
            PreviewRenderResult* result = nullptr);

        /**
         * @brief Builds line upload data from the wire preview mesh.
         *
         * @param preview Source operation preview.
         * @param options Conversion options.
         * @param result Optional diagnostic output.
         * @return CPU-side line upload data.
         */
        [[nodiscard]]
        static graphics::MeshUploadData build_wire_upload_data(
            const kernel::modeling::OperationPreview& preview,
            const PreviewRenderOptions& options = {},
            PreviewRenderResult* result = nullptr);

        /**
         * @brief Builds render objects from already resolved GPU meshes.
         *
         * The GPU meshes remain owned by the caller, normally a render cache or
         * synchronization component.
         *
         * @param sourceNode Mesh node whose transform is used by the preview.
         * @param preview Source operation preview.
         * @param solidMesh Resolved solid GPU mesh, or null.
         * @param wireMesh Resolved wire GPU mesh, or null.
         * @param options Render object options.
         * @param result Optional diagnostic output.
         * @return Generated solid and wire render objects.
         */
        [[nodiscard]]
        static PreviewRenderObjects build_render_objects(
            const MeshNode& sourceNode,
            const kernel::modeling::OperationPreview& preview,
            const graphics::GpuMesh* solidMesh,
            const graphics::GpuMesh* wireMesh,
            const PreviewRenderOptions& options = {},
            PreviewRenderResult* result = nullptr);

    private:
        /**
         * @brief Checks whether a preview may be converted.
         *
         * @param preview Source operation preview.
         * @param result Optional diagnostic output.
         * @return True when the preview is ready and valid.
         */
        [[nodiscard]]
        static bool validate_preview(
            const kernel::modeling::OperationPreview& preview,
            PreviewRenderResult* result);

        /**
         * @brief Copies editor transform data to graphics transform data.
         *
         * @param node Source mesh node.
         * @return Render transform.
         */
        [[nodiscard]]
        static graphics::RenderTransform build_render_transform(
            const MeshNode& node);

        /**
         * @brief Builds visibility flags suitable for transient previews.
         *
         * @param node Source mesh node.
         * @return Preview render visibility.
         */
        [[nodiscard]]
        static graphics::RenderVisibility build_render_visibility(
            const MeshNode& node);

        /**
         * @brief Writes a diagnostic message to an optional result.
         *
         * @param result Optional result object.
         * @param message Message to write.
         */
        static void set_message(
            PreviewRenderResult* result,
            std::string message);
    };

} // namespace locus::editor