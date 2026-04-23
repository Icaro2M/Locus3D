#include "graphics/Shader.h"

#include <glad/glad.h>
#include <glm/glm/gtc/type_ptr.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>

Shader::Shader(const std::string& vertexPath, const std::string& fragmentPath)
{
    std::string vertexCode = loadFile(vertexPath);
    std::string fragmentCode = loadFile(fragmentPath);

    unsigned int vertexShader = compileShader(vertexCode, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragmentCode, GL_FRAGMENT_SHADER);

    programID = glCreateProgram();

    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    int success;
    char infoLog[512];

    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programID, 512, nullptr, infoLog);
        std::cerr << "Shader linking error:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    glDeleteProgram(programID);
}

void Shader::use() const
{
    glUseProgram(programID);
}

unsigned int Shader::getID() const
{
    return programID;
}

void Shader::setMat4(const std::string& name, const glm::mat4& matrix) const
{
    int location = glGetUniformLocation(programID, name.c_str());

    if (location == -1)
    {
        std::cerr << "Warning: uniform '" << name << "' not found in shader program.\n";
    }

    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::setFloat(const std::string& name, float value) const
{
    int location = glGetUniformLocation(programID, name.c_str());

    if (location == -1)
    {
        std::cerr << "Warning: uniform '" << name << "' not found in shader program.\n";
    }

    glUniform1f(location, value);
}

std::string Shader::loadFile(const std::string& path)
{
    std::ifstream file(path);

    if (!file)
    {
        throw std::runtime_error("Failed to open shader file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

unsigned int Shader::compileShader(const std::string& source, unsigned int type)
{
    unsigned int shader = glCreateShader(type);

    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);

        if (type == GL_VERTEX_SHADER)
        {
            std::cerr << "Vertex shader compilation error:\n" << infoLog << std::endl;
        }
        else if (type == GL_FRAGMENT_SHADER)
        {
            std::cerr << "Fragment shader compilation error:\n" << infoLog << std::endl;
        }
        else
        {
            std::cerr << "Shader compilation error:\n" << infoLog << std::endl;
        }
    }

    return shader;
}