#pragma once

#include "graphics/appearance/VisualMaterial.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>

namespace locus::graphics
{
    class VisualMaterialLibrary
    {
    public:
        VisualMaterialLibrary() = default;
        ~VisualMaterialLibrary() = default;

        VisualMaterialLibrary(const VisualMaterialLibrary&) = delete;
        VisualMaterialLibrary& operator=(const VisualMaterialLibrary&) = delete;

        VisualMaterialLibrary(VisualMaterialLibrary&&) noexcept = default;
        VisualMaterialLibrary& operator=(VisualMaterialLibrary&&) noexcept = default;

        void add(VisualMaterial material)
        {
            materials_[material.name] = std::move(material);
        }

        [[nodiscard]] const VisualMaterial* find(const std::string& name) const
        {
            const auto it = materials_.find(name);

            if (it == materials_.end())
            {
                return nullptr;
            }

            return &it->second;
        }

        [[nodiscard]] bool contains(const std::string& name) const
        {
            return materials_.find(name) != materials_.end();
        }

        void clear()
        {
            materials_.clear();
        }

        [[nodiscard]] std::size_t size() const
        {
            return materials_.size();
        }

    private:
        std::unordered_map<std::string, VisualMaterial> materials_;
    };
}