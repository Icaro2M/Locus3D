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
     * @brief Describes one measurement line draw request.
     */
    struct MeasurementDrawItem
    {
        glm::vec3 start{ 0.0f, 0.0f, 0.0f };
        glm::vec3 end{ 1.0f, 0.0f, 0.0f };
        ColorRGBA color{ 1.0f, 0.85f, 0.15f, 1.0f };
        bool drawTicks = true;
    };

    /**
     * @brief Configuration used to create the measurement overlay object.
     */
    struct MeasurementRendererConfig
    {
        RenderObject::Id objectId = 1005;
        std::string objectName = "Measurements";
        RenderLayer layer = RenderLayer::Overlay;
        ColorRGBA defaultColor{ 1.0f, 0.85f, 0.15f, 1.0f };
        float tickLength = 0.18f;
    };

    /**
     * @brief Renders measurement lines with optional endpoint ticks.
     *
     * MeasurementRenderer records world-space measurement segments, expands
     * them into dynamic line geometry during upload, and submits a
     * non-selectable overlay object to the render scene.
     *
     * @note Very short measurements are ignored to avoid unstable tick axes.
     */
    class MeasurementRenderer
    {
    public:
        MeasurementRenderer() = default;
        ~MeasurementRenderer();

        MeasurementRenderer(const MeasurementRenderer&) = delete;
        MeasurementRenderer& operator=(const MeasurementRenderer&) = delete;

        MeasurementRenderer(MeasurementRenderer&& other) noexcept;
        MeasurementRenderer& operator=(MeasurementRenderer&& other) noexcept;

        /**
         * @brief Initializes the renderer with its shader and render metadata.
         *
         * @param shaderManager Shader registry used to resolve the debug line shader.
         * @param config Object id, object name, layer, default color, and tick length.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> create(
            const ShaderManager& shaderManager,
            const MeasurementRendererConfig& config = {}
        );

        /**
         * @brief Releases GPU resources and resets renderer state.
         */
        void destroy();

        /**
         * @brief Clears queued measurements and destroys the uploaded mesh.
         */
        void clear();

        /**
         * @brief Adds a measurement using the configured default color and ticks.
         *
         * @param start Measurement start point in world space.
         * @param end Measurement end point in world space.
         */
        void add_measurement(const glm::vec3& start, const glm::vec3& end);

        /**
         * @brief Adds a colored measurement with endpoint ticks.
         *
         * @param start Measurement start point in world space.
         * @param end Measurement end point in world space.
         * @param color Vertex color for the measurement.
         */
        void add_measurement(
            const glm::vec3& start,
            const glm::vec3& end,
            const ColorRGBA& color
        );
        /**
         * @brief Adds a colored measurement with configurable endpoint ticks.
         *
         * @param start Measurement start point in world space.
         * @param end Measurement end point in world space.
         * @param color Vertex color for the measurement.
         * @param drawTicks True to draw perpendicular ticks at both endpoints.
         */
        void add_measurement(
            const glm::vec3& start,
            const glm::vec3& end,
            const ColorRGBA& color,
            bool drawTicks
        );
        /**
         * @brief Adds a measurement from a draw item.
         *
         * @param item Measurement endpoints, color, and tick setting.
         */
        void add_measurement(const MeasurementDrawItem& item);

        /**
         * @brief Uploads all queued measurements as a dynamic line mesh.
         *
         * @param uploader Mesh uploader used to create the GPU mesh.
         * @return Empty result on success, or a graphics error on failure.
         */
        [[nodiscard]] GraphicsResult<void> upload(const MeshUploader& uploader);

        /**
         * @brief Submits the uploaded overlay object to a render scene.
         *
         * @param scene Scene that receives the measurement render object.
         */
        void submit(RenderScene& scene) const;

        /**
         * @brief Checks whether the renderer currently has a drawable object.
         *
         * @return True when the internal render object is drawable.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Checks whether any measurements are queued.
         *
         * @return True when at least one measurement has been added.
         */
        [[nodiscard]] bool has_measurements() const;

        /**
         * @brief Returns the number of queued measurements.
         *
         * @return Measurement count.
         */
        [[nodiscard]] std::size_t measurement_count() const;

        /**
         * @brief Returns the render object used for submission.
         *
         * @return Const reference to the internal render object.
         */
        [[nodiscard]] const RenderObject& render_object() const;

    private:
        static MeshVertex make_vertex(const glm::vec3& position, const ColorRGBA& color);
        static glm::vec3 build_tick_axis(const glm::vec3& direction);

        void append_line(
            std::vector<MeshVertex>& vertices,
            const glm::vec3& start,
            const glm::vec3& end,
            const ColorRGBA& color
        ) const;

        void append_measurement(
            std::vector<MeshVertex>& vertices,
            const MeasurementDrawItem& item
        ) const;

    private:
        MeasurementRendererConfig config_{};
        const Shader* shader_ = nullptr;
        std::vector<MeasurementDrawItem> measurements_;
        GpuMesh mesh_;
        RenderObject object_{};
    };
}
