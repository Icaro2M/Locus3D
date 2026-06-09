/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/gpu/Shader.h"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace locus::graphics
{
    /**
     * @brief Owns and caches shader programs loaded from the shader asset directory.
     */
    class ShaderManager
    {
    public:
        /**
         * @brief Creates an empty shader manager.
         */
        ShaderManager() = default;

        /**
         * @brief Releases all loaded shader programs.
         */
        ~ShaderManager();

        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;

        ShaderManager(ShaderManager&&) = delete;
        ShaderManager& operator=(ShaderManager&&) = delete;

        /**
         * @brief Sets the root directory used to resolve shader file paths.
         *
         * @param shaderRoot Directory that contains shader asset files.
         */
        void set_shader_root(std::filesystem::path shaderRoot);

        /**
         * @brief Returns the configured shader root directory.
         *
         * @return Shader root path.
         */
        [[nodiscard]] const std::filesystem::path& shader_root() const;

        /**
         * @brief Loads a shader program if it is not already cached.
         *
         * @param name Logical shader name used for later lookup.
         * @param vertexRelativePath Vertex shader path relative to the shader root.
         * @param fragmentRelativePath Fragment shader path relative to the shader root.
         * @return Pointer to the cached shader or an error.
         */
        [[nodiscard]] GraphicsResult<const Shader*> load(
            const std::string& name,
            const std::filesystem::path& vertexRelativePath,
            const std::filesystem::path& fragmentRelativePath
        );

        /**
         * @brief Finds a previously loaded shader.
         *
         * @param name Logical shader name.
         * @return Shader pointer, or nullptr when not found.
         */
        [[nodiscard]] const Shader* find(const std::string& name) const;

        /**
         * @brief Destroys all cached shader programs and clears the cache.
         */
        void clear();

    private:
        /**
         * @brief Resolves a shader path against the configured shader root.
         *
         * @param relativePath Relative or absolute shader path.
         * @return Absolute or root-relative shader path.
         */
        [[nodiscard]] std::filesystem::path resolve_shader_path(
            const std::filesystem::path& relativePath
        ) const;

    private:
        std::filesystem::path shaderRoot_;
        std::unordered_map<std::string, Shader> shaders_;
    };
}