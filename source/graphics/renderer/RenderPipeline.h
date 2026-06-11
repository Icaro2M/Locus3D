/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/renderer/DrawList.h"
#include "graphics/renderer/RenderQueue.h"
#include "graphics/renderer/RenderStats.h"
#include "graphics/renderer/Renderer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <cstddef>

namespace locus::graphics
{
    /**
     * @brief Coordinates per-frame render submission, queue building, and drawing.
     *
     * RenderPipeline stores submitted objects in a DrawList, lazily rebuilds the
     * RenderQueue when submissions change, and exposes the renderer statistics
     * captured after the last render call.
     */
    class RenderPipeline
    {
    public:
        RenderPipeline() = default;
        ~RenderPipeline() = default;

        RenderPipeline(const RenderPipeline&) = delete;
        RenderPipeline& operator=(const RenderPipeline&) = delete;

        RenderPipeline(RenderPipeline&&) noexcept = default;
        RenderPipeline& operator=(RenderPipeline&&) noexcept = default;

        /**
         * @brief Clears per-frame submissions, queue data, and render statistics.
         */
        void begin_frame();

        /**
         * @brief Reserves storage for an expected object count.
         *
         * @param objectCount Number of render objects expected this frame.
         */
        void reserve(std::size_t objectCount);

        /**
         * @brief Submits one render object for the current frame.
         *
         * @param object Object to submit.
         * @return Reference to the stored object.
         */
        RenderObject& submit(RenderObject object);

        /**
         * @brief Submits every object from a scene.
         *
         * @param scene Source scene.
         */
        void submit(const RenderScene& scene);

        /**
         * @brief Submits every object from another draw list.
         *
         * @param drawList Source draw list.
         */
        void submit(const DrawList& drawList);

        /**
         * @brief Rebuilds the render queue when submitted objects changed.
         */
        void build_queue();

        /**
         * @brief Renders the current queue and stores the resulting statistics.
         *
         * @param renderer Renderer used to execute draw commands.
         */
        void render(Renderer& renderer);

        /**
         * @brief Checks whether no objects were submitted.
         *
         * @return True when the draw list is empty.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Returns the number of submitted objects.
         *
         * @return Object count.
         */
        [[nodiscard]] std::size_t object_count() const;

        /**
         * @brief Returns the number of commands in the current queue.
         *
         * @return Render command count.
         */
        [[nodiscard]] std::size_t command_count() const;

        /**
         * @brief Returns the current draw list.
         *
         * @return Const reference to submitted render objects.
         */
        [[nodiscard]] const DrawList& draw_list() const;

        /**
         * @brief Returns the current render queue.
         *
         * @return Const reference to the render queue.
         */
        [[nodiscard]] const RenderQueue& queue() const;

        /**
         * @brief Returns statistics captured after the last render call.
         *
         * @return Const reference to render statistics.
         */
        [[nodiscard]] const RenderStats& stats() const;

    private:
        DrawList drawList_;
        RenderQueue queue_;
        RenderStats stats_{};
        bool queueDirty_ = true;
    };
}
