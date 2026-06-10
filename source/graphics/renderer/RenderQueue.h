/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/renderer/RenderCommand.h"

#include <cstddef>
#include <vector>

namespace locus::graphics
{
    class RenderScene;
    struct RenderObject;

    /**
     * @brief Collects and orders render commands for a frame.
     */
    class RenderQueue
    {
    public:
        /**
         * @brief Contiguous command storage type.
         */
        using CommandList = std::vector<RenderCommand>;

        /**
         * @brief Creates an empty render queue.
         */
        RenderQueue() = default;

        /**
         * @brief Destroys queued command storage.
         */
        ~RenderQueue() = default;

        RenderQueue(const RenderQueue&) = default;
        RenderQueue& operator=(const RenderQueue&) = default;

        RenderQueue(RenderQueue&&) noexcept = default;
        RenderQueue& operator=(RenderQueue&&) noexcept = default;

        /**
         * @brief Removes all queued commands and resets sequence numbering.
         */
        void clear();

        /**
         * @brief Reserves storage for future commands.
         *
         * @param count Number of commands to reserve.
         */
        void reserve(std::size_t count);

        /**
         * @brief Adds one render object to the queue.
         *
         * @param object Object referenced by the queued command.
         */
        void add_object(const RenderObject& object);

        /**
         * @brief Rebuilds the queue from every object in a scene.
         *
         * @param scene Source render scene.
         */
        void build_from_scene(const RenderScene& scene);

        /**
         * @brief Sorts commands by layer priority while preserving insertion order.
         */
        void sort();

        /**
         * @brief Checks whether the queue has no commands.
         *
         * @return True when the command list is empty.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Returns the number of queued commands.
         *
         * @return Command count.
         */
        [[nodiscard]] std::size_t command_count() const;

        /**
         * @brief Returns all queued commands.
         *
         * @return Read-only command list.
         */
        [[nodiscard]] const CommandList& commands() const;

    private:
        [[nodiscard]] static u32 priority_for_layer(RenderLayer layer);

    private:
        CommandList commands_;
        u32 nextSequence_ = 0;
    };
}
