/*
 * SPDX-FileCopyrightText: 2026 Icaro
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/renderer/RenderQueue.h"
#include "graphics/scene/RenderLayer.h"
#include "graphics/scene/RenderObject.h"
#include "graphics/scene/RenderScene.h"

#include <cstddef>
#include <vector>

namespace locus::graphics
{
    /**
     * @brief Collects render objects before building a sorted render queue.
     *
     * DrawList is a lightweight staging container for objects submitted from
     * scenes, overlays, and tools before they are converted into draw commands.
     */
    class DrawList
    {
    public:
        using ObjectList = std::vector<RenderObject>;

        DrawList() = default;
        ~DrawList() = default;

        DrawList(const DrawList&) = default;
        DrawList& operator=(const DrawList&) = default;

        DrawList(DrawList&&) noexcept = default;
        DrawList& operator=(DrawList&&) noexcept = default;

        /**
         * @brief Removes all queued render objects.
         */
        void clear();

        /**
         * @brief Reserves storage for render objects.
         *
         * @param count Desired object capacity.
         */
        void reserve(std::size_t count);

        /**
         * @brief Adds one render object.
         *
         * @param object Object to append.
         * @return Reference to the stored object.
         */
        RenderObject& add_object(RenderObject object);

        /**
         * @brief Appends all objects from a scene.
         *
         * @param scene Source scene.
         */
        void add_objects(const RenderScene& scene);

        /**
         * @brief Appends all objects from a list.
         *
         * @param objects Source object list.
         */
        void add_objects(const ObjectList& objects);

        /**
         * @brief Appends this draw list to an existing scene.
         *
         * @param scene Destination scene.
         */
        void append_to_scene(RenderScene& scene) const;

        /**
         * @brief Copies this draw list into a new scene.
         *
         * @return RenderScene containing the queued objects.
         */
        [[nodiscard]] RenderScene to_scene() const;

        /**
         * @brief Builds a sorted render queue from this draw list.
         *
         * @return Sorted render queue.
         */
        [[nodiscard]] RenderQueue build_queue() const;

        /**
         * @brief Rebuilds an existing render queue from this draw list.
         *
         * @param queue Queue to clear, fill, and sort.
         */
        void build_queue(RenderQueue& queue) const;

        /**
         * @brief Copies objects that belong to a specific render layer.
         *
         * @param layer Layer to filter by.
         * @return Object list containing only matching objects.
         */
        [[nodiscard]] ObjectList objects_in_layer(RenderLayer layer) const;

        /**
         * @brief Checks whether the list has no objects.
         *
         * @return True when empty.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Returns the number of stored render objects.
         *
         * @return Object count.
         */
        [[nodiscard]] std::size_t object_count() const;

        /**
         * @brief Returns all stored render objects.
         *
         * @return Const reference to the object list.
         */
        [[nodiscard]] const ObjectList& objects() const;

    private:
        ObjectList objects_;
    };
}
