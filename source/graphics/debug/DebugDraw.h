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
#include <vector>

namespace locus::graphics
{
    /**
     * @brief Configuration used to create the debug line draw object.
     */
    struct DebugDrawConfig
    {
        RenderObject::Id objectId = 9001;
        std::string objectName = "DebugDraw";
        RenderLayer layer = RenderLayer::Debug;
        ColorRGBA defaultColor{ 1.0f, 0.85f, 0.15f, 1.0f };
    };

    /**
     * @brief Builds transient line geometry for debug visualization.
     *
     * DebugDraw accumulates CPU-side line vertices, uploads them as a dynamic
     * GPU mesh, and submits a non-selectable render object to a scene.
     *
     * @note Call clear() before recording geometry for a new debug frame.
     */
    class DebugDraw
    {
    public:
        DebugDraw() = default;
        ~DebugDraw();

        DebugDraw(const DebugDraw&) = delete;
        DebugDraw& operator=(const DebugDraw&) = delete;

        DebugDraw(DebugDraw&& other) noexcept;
        DebugDraw& operator=(DebugDraw&& other) noexcept;

        /**
         * @brief Initializes the debug draw object with its shader and render metadata.
         *
         * @param shaderManager Shader registry used to resolve the debug/draw shader.
         * @param config Object id, object name, layer, and default line color.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const DebugDrawConfig& config = {}
        );

        /**
         * @brief Releases GPU resources and resets the debug draw state.
         */
        void destroy();

        /**
         * @brief Clears recorded lines and destroys the uploaded mesh.
         */
        void clear();

        /**
         * @brief Adds a line using the configured default color.
         *
         * @param a First endpoint in world space.
         * @param b Second endpoint in world space.
         */
        void add_line(const glm::vec3& a, const glm::vec3& b);

        /**
         * @brief Adds a colored line segment.
         *
         * @param a First endpoint in world space.
         * @param b Second endpoint in world space.
         * @param color Vertex color for the line.
         */
        void add_line(const glm::vec3& a, const glm::vec3& b, const ColorRGBA& color);

        /**
         * @brief Adds a ray using the configured default color.
         *
         * @param origin Ray start point in world space.
         * @param direction Ray direction. It does not need to be normalized.
         * @param length Ray length in world units.
         */
        void add_ray(
            const glm::vec3& origin,
            const glm::vec3& direction,
            float length
        );

        /**
         * @brief Adds a colored ray.
         *
         * @param origin Ray start point in world space.
         * @param direction Ray direction. It does not need to be normalized.
         * @param length Ray length in world units.
         * @param color Vertex color for the ray.
         */
        void add_ray(
            const glm::vec3& origin,
            const glm::vec3& direction,
            float length,
            const ColorRGBA& color
        );

        /**
         * @brief Adds a wire box using the configured default color.
         *
         * @param minPoint Minimum corner of the box.
         * @param maxPoint Maximum corner of the box.
         */
        void add_box(
            const glm::vec3& minPoint,
            const glm::vec3& maxPoint
        );

        /**
         * @brief Adds a colored wire box.
         *
         * @param minPoint Minimum corner of the box.
         * @param maxPoint Maximum corner of the box.
         * @param color Vertex color for all box edges.
         */
        void add_box(
            const glm::vec3& minPoint,
            const glm::vec3& maxPoint,
            const ColorRGBA& color
        );

        /**
         * @brief Uploads the recorded line geometry to the GPU.
         *
         * @param uploader Mesh uploader used to create the dynamic line mesh.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> upload(const MeshUploader& uploader);

        /**
         * @brief Submits the uploaded debug object to a render scene.
         *
         * @param scene Scene that receives the debug render object.
         */
        void submit(RenderScene& scene) const;

        /**
         * @brief Checks whether the debug draw object can be rendered.
         *
         * @return True when it has a drawable render object.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Checks whether CPU-side debug geometry has been recorded.
         *
         * @return True when at least one line vertex exists.
         */
        [[nodiscard]] bool has_geometry() const;

        /**
         * @brief Returns the number of recorded line segments.
         *
         * @return Line segment count.
         */
        [[nodiscard]] std::size_t line_count() const;

        /**
         * @brief Returns the render object used for submission.
         *
         * @return Const reference to the internal render object.
         */
        [[nodiscard]] const RenderObject& render_object() const;

    private:
        static MeshVertex make_vertex(const glm::vec3& position, const ColorRGBA& color);

    private:
        DebugDrawConfig config_{};
        const Shader* shader_ = nullptr;
        std::vector<MeshVertex> vertices_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}
