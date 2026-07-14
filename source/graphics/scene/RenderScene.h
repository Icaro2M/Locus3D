/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/scene/RenderLayer.h"
#include "graphics/scene/RenderObject.h"

#include <cstddef>
#include <vector>

namespace locus::graphics
{
    /**
     * @brief Owns the list of render objects submitted to a renderer.
     */
    class RenderScene
    {
    public:
        /**
         * @brief Contiguous storage type used for render objects.
         */
        using ObjectList = std::vector<RenderObject>;

        /**
         * @brief Creates an empty render scene.
         */
        RenderScene() = default;

        /**
         * @brief Destroys the scene object list.
         */
        ~RenderScene() = default;

        RenderScene(const RenderScene&) = default;
        RenderScene& operator=(const RenderScene&) = default;

        RenderScene(RenderScene&&) noexcept = default;
        RenderScene& operator=(RenderScene&&) noexcept = default;

        /**
         * @brief Adds an object to the scene.
         *
         * @param object Object data to move into the scene.
         * @return Reference to the stored object.
         */
        RenderObject& add_object(RenderObject object);

        /**
         * @brief Removes all objects from the scene.
         */
        void clear();

        /**
         * @brief Reserves storage for future objects.
         *
         * @param count Number of objects to reserve.
         */
        void reserve(std::size_t count);

        /**
         * @brief Checks whether the scene has no objects.
         *
         * @return True when no objects are stored.
         */
        [[nodiscard]] bool empty() const;

        /**
         * @brief Returns the number of stored render objects.
         *
         * @return Object count.
         */
        [[nodiscard]] std::size_t object_count() const;

        /**
         * @brief Returns all render objects for mutation.
         *
         * @return Mutable object list.
         */
        [[nodiscard]] ObjectList& objects();

        /**
         * @brief Returns all render objects.
         *
         * @return Read-only object list.
         */
        [[nodiscard]] const ObjectList& objects() const;

        /**
         * @brief Copies the objects that belong to a specific render layer.
         *
         * @param layer Layer to filter by.
         * @return Object list containing only matching objects.
         */
        [[nodiscard]] ObjectList objects_in_layer(RenderLayer layer) const;



    private:
        ObjectList objects_;
    };
}
