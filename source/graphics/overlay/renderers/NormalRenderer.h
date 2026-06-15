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
     * @brief Describes one normal vector draw request.
     */
    struct NormalDrawItem
    {
        glm::vec3 origin{ 0.0f, 0.0f, 0.0f };
        glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
        float length = 1.0f;
        ColorRGBA color{ 0.35f, 0.75f, 1.0f, 1.0f };
    };

    /**
     * @brief Configuration used to create the normal overlay object.
     */
    struct NormalRendererConfig
    {
        RenderObject::Id objectId = 1004;
        std::string objectName = "Normals";
        RenderLayer layer = RenderLayer::Overlay;
        float defaultLength = 0.25f;
        ColorRGBA defaultColor{ 0.35f, 0.75f, 1.0f, 1.0f };
    };

    /**
     * @brief Renders normal vectors as world-space line segments.
     *
     * NormalRenderer stores normal draw requests on the CPU, expands them into
     * dynamic line vertices during upload, and submits a non-selectable overlay
     * object to the render scene.
     *
     * @note Call clear() before recording normals for a new overlay frame.
     */
    class NormalRenderer
    {
    public:
        NormalRenderer() = default;
        ~NormalRenderer();

        NormalRenderer(const NormalRenderer&) = delete;
        NormalRenderer& operator=(const NormalRenderer&) = delete;

        NormalRenderer(NormalRenderer&& other) noexcept;
        NormalRenderer& operator=(NormalRenderer&& other) noexcept;

        /**
         * @brief Initializes the renderer with its shader and render metadata.
         *
         * @param shaderManager Shader registry used to resolve the debug line shader.
         * @param config Object id, object name, layer, default length, and color.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const NormalRendererConfig& config = {}
        );

        /**
         * @brief Releases GPU resources and resets renderer state.
         */
        void destroy();

        /**
         * @brief Clears queued normals and destroys the uploaded mesh.
         */
        void clear();

        /**
         * @brief Adds a normal using the configured default length and color.
         *
         * @param origin Normal start point in world space.
         * @param normal Normal direction. It does not need to be normalized.
         */
        void add_normal(const glm::vec3& origin, const glm::vec3& normal);

        /**
         * @brief Adds a normal using the configured default color.
         *
         * @param origin Normal start point in world space.
         * @param normal Normal direction. It does not need to be normalized.
         * @param length Line length in world units.
         */
        void add_normal(const glm::vec3& origin, const glm::vec3& normal, float length);

        /**
         * @brief Adds a colored normal line.
         *
         * @param origin Normal start point in world space.
         * @param normal Normal direction. It does not need to be normalized.
         * @param length Line length in world units.
         * @param color Vertex color for the normal line.
         */
        void add_normal(
            const glm::vec3& origin,
            const glm::vec3& normal,
            float length,
            const ColorRGBA& color
        );
        /**
         * @brief Adds a normal from a draw item.
         *
         * @param item Normal origin, direction, length, and color.
         */
        void add_normal(const NormalDrawItem& item);

        /**
         * @brief Uploads all queued normals as a dynamic line mesh.
         *
         * @param uploader Mesh uploader used to create the GPU mesh.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> upload(const MeshUploader& uploader);

        /**
         * @brief Submits the uploaded overlay object to a render scene.
         *
         * @param scene Scene that receives the normal render object.
         */
        void submit(RenderScene& scene) const;

        /**
         * @brief Checks whether the renderer currently has a drawable object.
         *
         * @return True when the internal render object is drawable.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Checks whether any normals are queued.
         *
         * @return True when at least one normal has been added.
         */
        [[nodiscard]] bool has_normals() const;

        /**
         * @brief Returns the number of queued normals.
         *
         * @return Normal count.
         */
        [[nodiscard]] std::size_t normal_count() const;

        /**
         * @brief Returns the render object used for submission.
         *
         * @return Const reference to the internal render object.
         */
        [[nodiscard]] const RenderObject& render_object() const;

    private:
        static MeshVertex make_vertex(const glm::vec3& position, const ColorRGBA& color);

    private:
        NormalRendererConfig config_{};
        const Shader* shader_ = nullptr;
        std::vector<NormalDrawItem> normals_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}
