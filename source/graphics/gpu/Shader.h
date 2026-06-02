/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "graphics/common/GraphicsResult.h"
#include "graphics/common/GraphicsTypes.h"

#include <string>

namespace locus::graphics
{
    /**
     * @brief RAII wrapper for an OpenGL shader program.
     */
    class Shader
    {
    public:
        /**
         * @brief Creates an empty shader program wrapper.
         */
        Shader() = default;

        /**
         * @brief Deletes the owned shader program, if any.
         */
        ~Shader();

        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;

        Shader(Shader&& other) noexcept;
        Shader& operator=(Shader&& other) noexcept;

        /**
         * @brief Compiles and links a vertex/fragment shader program from source.
         *
         * @param vertexSource Vertex shader source code.
         * @param fragmentSource Fragment shader source code.
         * @return Success or shader compilation/linking error.
         */
        [[nodiscard]] GraphicsResult<void> create_from_source(
            const std::string& vertexSource,
            const std::string& fragmentSource);

        /**
         * @brief Loads, compiles, and links a vertex/fragment shader program.
         *
         * @param vertexPath Vertex shader file path.
         * @param fragmentPath Fragment shader file path.
         * @return Success or shader loading/compilation/linking error.
         */
        [[nodiscard]] GraphicsResult<void> create_from_files(
            const std::string& vertexPath,
            const std::string& fragmentPath);

        /**
         * @brief Deletes the owned shader program.
         */
        void destroy();

        /**
         * @brief Binds the shader program for rendering.
         */
        void bind() const;

        /**
         * @brief Unbinds any current shader program.
         */
        void unbind() const;

        /**
         * @brief Checks whether this wrapper owns a shader program.
         *
         * @return True when the OpenGL program ID is non-zero.
         */
        [[nodiscard]] bool is_valid() const;

        /**
         * @brief Returns the OpenGL shader program ID.
         *
         * @return OpenGL object ID.
         */
        [[nodiscard]] u32 id() const;

        /**
         * @brief Sets an integer uniform value.
         *
         * @param name Uniform name.
         * @param value Uniform value.
         */
        void set_int(const std::string& name, int value) const;

        /**
         * @brief Sets a float uniform value.
         *
         * @param name Uniform name.
         * @param value Uniform value.
         */
        void set_float(const std::string& name, float value) const;

        /**
         * @brief Sets a vec2 uniform value.
         *
         * @param name Uniform name.
         * @param x First component.
         * @param y Second component.
         */
        void set_vec2(const std::string& name, float x, float y) const;

        /**
         * @brief Sets a vec3 uniform value.
         *
         * @param name Uniform name.
         * @param x First component.
         * @param y Second component.
         * @param z Third component.
         */
        void set_vec3(const std::string& name, float x, float y, float z) const;

        /**
         * @brief Sets a vec4 uniform value.
         *
         * @param name Uniform name.
         * @param x First component.
         * @param y Second component.
         * @param z Third component.
         * @param w Fourth component.
         */
        void set_vec4(const std::string& name, float x, float y, float z, float w) const;

        /**
         * @brief Sets a 4x4 matrix uniform value.
         *
         * @param name Uniform name.
         * @param value Pointer to 16 contiguous float values.
         */
        void set_mat4(const std::string& name, const float* value) const;

    private:
        [[nodiscard]] GraphicsResult<u32> compile_shader(
            ShaderStage stage,
            const std::string& source) const;

        [[nodiscard]] GraphicsResult<void> link_program(u32 vertexShader, u32 fragmentShader);

        [[nodiscard]] GraphicsResult<std::string> read_file(const std::string& path) const;

        [[nodiscard]] int uniform_location(const std::string& name) const;

    private:
        u32 id_ = 0;
    };

}
