#pragma once

#include <string>
#include <glad/glad.h>

class Shader
{
public:

    Shader(const std::string& vertexPath,
        const std::string& fragmentPath);

    ~Shader();

    void use() const;

    unsigned int getID() const;

private:

    unsigned int programID;

    std::string loadFile(const std::string& path);
    unsigned int compileShader(const std::string& source, unsigned int type);
};