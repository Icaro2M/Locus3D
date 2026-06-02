/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "graphics/gpu/Shader.h"

#include "graphics/common/GraphicsError.h"

#include <glad/glad.h>

#include <fstream>
#include <sstream>
#include <utility>

namespace locus::graphics
{

    namespace
    {
        /*
         * Shader stage translation is kept private to avoid exposing OpenGL
         * constants through the graphics layer interface.
         */
        u32 to_gl_shader_stage(ShaderStage stage)
        {
            switch (stage)
            {
            case ShaderStage::Vertex:
                return GL_VERTEX_SHADER;

            case ShaderStage::Fragment:
                return GL_FRAGMENT_SHADER;

            case ShaderStage::Geometry:
                return GL_GEOMETRY_SHADER;
            }

            return GL_VERTEX_SHADER;
        }

        const char* shader_stage_name(ShaderStage stage)
        {
            switch (stage)
            {
            case ShaderStage::Vertex:
                return "vertex";

            case ShaderStage::Fragment:
                return "fragment";

            case ShaderStage::Geometry:
                return "geometry";
            }

            return "unknown";
        }

        std::string shader_info_log(u32 shader)
        {
            int length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

            /*
             * OpenGL includes the null terminator in the reported log length.
             * A length of 0 or 1 means there is no useful message to return.
             */
            if (length <= 1)
            {
                return {};
            }

            std::string log;
            log.resize(static_cast<std::size_t>(length));

            glGetShaderInfoLog(shader, length, nullptr, log.data());

            return log;
        }

        std::string program_info_log(u32 program)
        {
            int length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);

            if (length <= 1)
            {
                return {};
            }

            std::string log;
            log.resize(static_cast<std::size_t>(length));

            glGetProgramInfoLog(program, length, nullptr, log.data());

            return log;
        }

    } 

    Shader::~Shader()
    {
        destroy();
    }

    Shader::Shader(Shader&& other) noexcept
    {
        *this = std::move(other);
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        destroy();

        id_ = other.id_;
        other.id_ = 0;

        return *this;
    }

    GraphicsResult<void> Shader::create_from_source(
        const std::string& vertexSource,
        const std::string& fragmentSource)
    {
        if (id_ != 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::InvalidOperation,
                "Cannot create Shader because it already owns an OpenGL program.");
        }

        auto vertexShaderResult = compile_shader(ShaderStage::Vertex, vertexSource);

        if (!vertexShaderResult)
        {
            return vertexShaderResult.error();
        }

        auto fragmentShaderResult = compile_shader(ShaderStage::Fragment, fragmentSource);

        if (!fragmentShaderResult)
        {
            glDeleteShader(vertexShaderResult.value());
            return fragmentShaderResult.error();
        }

        const u32 vertexShader = vertexShaderResult.value();
        const u32 fragmentShader = fragmentShaderResult.value();

        auto linkResult = link_program(vertexShader, fragmentShader);

        /*
         * Once linked, shader objects are no longer needed; the program keeps
         * its compiled executable state.
         */
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        if (!linkResult)
        {
            return linkResult.error();
        }

        return {};
    }

    GraphicsResult<void> Shader::create_from_files(
        const std::string& vertexPath,
        const std::string& fragmentPath)
    {
        auto vertexSourceResult = read_file(vertexPath);

        if (!vertexSourceResult)
        {
            return vertexSourceResult.error();
        }

        auto fragmentSourceResult = read_file(fragmentPath);

        if (!fragmentSourceResult)
        {
            return fragmentSourceResult.error();
        }

        return create_from_source(vertexSourceResult.value(), fragmentSourceResult.value());
    }

    void Shader::destroy()
    {
        if (id_ != 0)
        {
            glDeleteProgram(id_);
            id_ = 0;
        }
    }

    void Shader::bind() const
    {
        glUseProgram(id_);
    }

    void Shader::unbind() const
    {
        glUseProgram(0);
    }

    bool Shader::is_valid() const
    {
        return id_ != 0;
    }

    u32 Shader::id() const
    {
        return id_;
    }

    void Shader::set_int(const std::string& name, int value) const
    {
        glUniform1i(uniform_location(name), value);
    }

    void Shader::set_float(const std::string& name, float value) const
    {
        glUniform1f(uniform_location(name), value);
    }

    void Shader::set_vec2(const std::string& name, float x, float y) const
    {
        glUniform2f(uniform_location(name), x, y);
    }

    void Shader::set_vec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(uniform_location(name), x, y, z);
    }

    void Shader::set_vec4(const std::string& name, float x, float y, float z, float w) const
    {
        glUniform4f(uniform_location(name), x, y, z, w);
    }

    void Shader::set_mat4(const std::string& name, const float* value) const
    {
        glUniformMatrix4fv(uniform_location(name), 1, GL_FALSE, value);
    }

    GraphicsResult<u32> Shader::compile_shader(
        ShaderStage stage,
        const std::string& source) const
    {
        if (source.empty())
        {
            std::ostringstream stream;
            stream << "Cannot compile empty " << shader_stage_name(stage) << " shader source.";

            return GraphicsError::make(
                GraphicsErrorCode::ShaderCompilationFailed,
                stream.str());
        }

        const u32 shader = glCreateShader(to_gl_shader_stage(stage));

        if (shader == 0)
        {
            std::ostringstream stream;
            stream << "Failed to create OpenGL " << shader_stage_name(stage) << " shader object.";

            return GraphicsError::make(
                GraphicsErrorCode::ShaderCompilationFailed,
                stream.str());
        }

        const char* sourcePointer = source.c_str();

        glShaderSource(shader, 1, &sourcePointer, nullptr);
        glCompileShader(shader);

        int success = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

        if (success != GL_TRUE)
        {
            const std::string log = shader_info_log(shader);
            glDeleteShader(shader);

            std::ostringstream stream;
            stream
                << "Failed to compile "
                << shader_stage_name(stage)
                << " shader.";

            if (!log.empty())
            {
                stream << "\n" << log;
            }

            return GraphicsError::make(
                GraphicsErrorCode::ShaderCompilationFailed,
                stream.str());
        }

        return shader;
    }

    GraphicsResult<void> Shader::link_program(u32 vertexShader, u32 fragmentShader)
    {
        const u32 program = glCreateProgram();

        if (program == 0)
        {
            return GraphicsError::make(
                GraphicsErrorCode::ShaderLinkFailed,
                "Failed to create OpenGL shader program.");
        }

        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        int success = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &success);

        if (success != GL_TRUE)
        {
            const std::string log = program_info_log(program);
            glDeleteProgram(program);

            std::ostringstream stream;
            stream << "Failed to link OpenGL shader program.";

            if (!log.empty())
            {
                stream << "\n" << log;
            }

            return GraphicsError::make(
                GraphicsErrorCode::ShaderLinkFailed,
                stream.str());
        }

        /*
         * Program ownership is assigned only after a successful link, so failed
         * creation never leaves this wrapper with a partially usable program.
         */
        id_ = program;

        return {};
    }

    GraphicsResult<std::string> Shader::read_file(const std::string& path) const
    {
        std::ifstream file(path);

        if (!file.is_open())
        {
            return GraphicsError::make(
                GraphicsErrorCode::ShaderFileReadFailed,
                "Failed to open shader file: " + path);
        }

        std::stringstream stream;
        stream << file.rdbuf();

        return stream.str();
    }

    int Shader::uniform_location(const std::string& name) const
    {
        return glGetUniformLocation(id_, name.c_str());
    }

}
