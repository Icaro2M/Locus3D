/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/gpu/ShaderManager.h"

#include "graphics/common/GraphicsError.h"

#include <utility>

namespace locus::graphics
{
    ShaderManager::~ShaderManager()
    {
        clear();
    }

    void ShaderManager::set_shader_root(std::filesystem::path shaderRoot)
    {
        shaderRoot_ = std::move(shaderRoot);
    }

    const std::filesystem::path& ShaderManager::shader_root() const
    {
        return shaderRoot_;
    }

    GraphicsResult<const Shader*> ShaderManager::load(
        const std::string& name,
        const std::filesystem::path& vertexRelativePath,
        const std::filesystem::path& fragmentRelativePath)
    {
        if (name.empty())
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidArgument,
                "Cannot load shader with empty name."
            );
        }

        if (shaderRoot_.empty())
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot load shader before setting shader root."
            );
        }

        auto existing = shaders_.find(name);

        if (existing != shaders_.end())
        {
            return &existing->second;
        }

        Shader shader;

        const std::filesystem::path vertexPath = resolve_shader_path(vertexRelativePath);
        const std::filesystem::path fragmentPath = resolve_shader_path(fragmentRelativePath);

        auto result = shader.create_from_files(
            vertexPath.string(),
            fragmentPath.string()
        );

        if (!result)
        {
            return result.error();
        }

        auto [iterator, inserted] = shaders_.emplace(name, std::move(shader));

        if (!inserted)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ResourceAlreadyExists,
                "Shader resource already exists: " + name
            );
        }

        return &iterator->second;
    }

    const Shader* ShaderManager::find(const std::string& name) const
    {
        const auto iterator = shaders_.find(name);

        if (iterator == shaders_.end())
        {
            return nullptr;
        }

        return &iterator->second;
    }

    void ShaderManager::clear()
    {
        for (auto& [name, shader] : shaders_)
        {
            shader.destroy();
        }

        shaders_.clear();
    }

    std::filesystem::path ShaderManager::resolve_shader_path(
        const std::filesystem::path& relativePath) const
    {
        if (relativePath.is_absolute())
        {
            return relativePath;
        }

        return shaderRoot_ / relativePath;
    }
}