/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/appearance/VisualMaterial.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

namespace locus::graphics
{
    /**
     * @brief Name-indexed collection of reusable visual materials.
     */
    class VisualMaterialLibrary
    {
    public:
        /**
         * @brief Creates an empty material library.
         */
        VisualMaterialLibrary() = default;

        /**
         * @brief Destroys stored material definitions.
         */
        ~VisualMaterialLibrary() = default;

        VisualMaterialLibrary(const VisualMaterialLibrary&) = delete;
        VisualMaterialLibrary& operator=(const VisualMaterialLibrary&) = delete;

        VisualMaterialLibrary(VisualMaterialLibrary&&) noexcept = default;
        VisualMaterialLibrary& operator=(VisualMaterialLibrary&&) noexcept = default;

        /**
         * @brief Adds or replaces a material by name.
         *
         * @param material Material to move into the library.
         */
        void add(VisualMaterial material)
        {
            materials_[material.name] = std::move(material);
        }

        /**
         * @brief Finds a material by name.
         *
         * @param name Material name.
         * @return Material pointer, or nullptr when not found.
         */
        [[nodiscard]] const VisualMaterial* find(const std::string& name) const
        {
            const auto it = materials_.find(name);

            if (it == materials_.end())
            {
                return nullptr;
            }

            return &it->second;
        }

        /**
         * @brief Checks whether a material name exists.
         *
         * @param name Material name.
         * @return True when the library contains the material.
         */
        [[nodiscard]] bool contains(const std::string& name) const
        {
            return materials_.find(name) != materials_.end();
        }

        /**
         * @brief Removes every material from the library.
         */
        void clear()
        {
            materials_.clear();
        }

        /**
         * @brief Returns the number of stored materials.
         *
         * @return Material count.
         */
        [[nodiscard]] std::size_t size() const
        {
            return materials_.size();
        }

    private:
        std::unordered_map<std::string, VisualMaterial> materials_;
    };
}
