/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"
#include "graphics/gpu/ShaderManager.h"
#include "graphics/mesh/GpuMesh.h"
#include "graphics/mesh/MeshUploader.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace locus::graphics
{
    /**
     * @brief Describes one wireframe bounding box draw request.
     */
    struct BoundingBoxDrawItem
    {
        glm::vec3 minPoint{ 0.0f, 0.0f, 0.0f };
        glm::vec3 maxPoint{ 0.0f, 0.0f, 0.0f };
        ColorRGBA color{ 0.2f, 0.85f, 1.0f, 1.0f };
    };

    /**
     * @brief Configuration used to create the bounding box overlay object.
     */
    struct BoundingBoxRendererConfig
    {
        RenderObject::Id objectId = 1003;
        std::string objectName = "BoundingBoxes";
        RenderLayer layer = RenderLayer::Overlay;
        ColorRGBA defaultColor{ 0.2f, 0.85f, 1.0f, 1.0f };
    };

    /**
     * @brief Renders one or more world-space bounding boxes as line geometry.
     *
     * BoundingBoxRenderer stores box requests on the CPU, expands them into
     * line vertices during upload, and submits a non-selectable overlay object.
     *
     * @note Call clear() before recording boxes for a new overlay frame.
     */
    class BoundingBoxRenderer
    {
    public:
        BoundingBoxRenderer() = default;
        ~BoundingBoxRenderer();

        BoundingBoxRenderer(const BoundingBoxRenderer&) = delete;
        BoundingBoxRenderer& operator=(const BoundingBoxRenderer&) = delete;

        BoundingBoxRenderer(BoundingBoxRenderer&& other) noexcept;
        BoundingBoxRenderer& operator=(BoundingBoxRenderer&& other) noexcept;

        /**
         * @brief Initializes the renderer with its shader and render metadata.
         *
         * @param shaderManager Shader registry used to resolve the overlay shader.
         * @param config Object id, object name, layer, and default box color.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const BoundingBoxRendererConfig& config = {}
        );

        /**
         * @brief Releases GPU resources and resets renderer state.
         */
        void destroy();

        /**
         * @brief Clears queued boxes and destroys the uploaded mesh.
         */
        void clear();

        /**
         * @brief Adds a box using the configured default color.
         *
         * @param minPoint Minimum corner of the box.
         * @param maxPoint Maximum corner of the box.
         */
        void add_box(const glm::vec3& minPoint, const glm::vec3& maxPoint);

        /**
         * @brief Adds a colored box.
         *
         * @param minPoint Minimum corner of the box.
         * @param maxPoint Maximum corner of the box.
         * @param color Vertex color for all box edges.
         */
        void add_box(const glm::vec3& minPoint, const glm::vec3& maxPoint, const ColorRGBA& color);

        /**
         * @brief Adds a box from a draw item.
         *
         * @param item Box bounds and color.
         */
        void add_box(const BoundingBoxDrawItem& item);

        /**
         * @brief Uploads all queued boxes as a dynamic line mesh.
         *
         * @param uploader Mesh uploader used to create the GPU mesh.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> upload(const MeshUploader& uploader);

        /**
         * @brief Submits the uploaded overlay object to a render scene.
         *
         * @param scene Scene that receives the bounding box render object.
         */
        void submit(RenderScene& scene) const;

        /**
         * @brief Checks whether the renderer currently has a drawable object.
         *
         * @return True when the internal render object is drawable.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Checks whether any boxes are queued.
         *
         * @return True when at least one box has been added.
         */
        [[nodiscard]] bool has_boxes() const;

        /**
         * @brief Returns the number of queued boxes.
         *
         * @return Box count.
         */
        [[nodiscard]] std::size_t box_count() const;

        /**
         * @brief Returns the render object used for submission.
         *
         * @return Const reference to the internal render object.
         */
        [[nodiscard]] const RenderObject& render_object() const;

    private:
        static MeshVertex make_vertex(const glm::vec3& position, const ColorRGBA& color);
        static void add_line(
            std::vector<MeshVertex>& vertices,
            const glm::vec3& a,
            const glm::vec3& b,
            const ColorRGBA& color
        );

        static void append_box_vertices(
            std::vector<MeshVertex>& vertices,
            const glm::vec3& minPoint,
            const glm::vec3& maxPoint,
            const ColorRGBA& color
        );

    private:
        BoundingBoxRendererConfig config_{};
        const Shader* shader_ = nullptr;
        std::vector<BoundingBoxDrawItem> boxes_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}
