#pragma once

#include <string>
#include <glm/glm/glm.hpp>

class Shader
{
private:
    unsigned int programID;

    std::string loadFile(const std::string& path);
    unsigned int compileShader(const std::string& source, unsigned int type);

public:
    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void use() const;
    unsigned int getID() const;

    void setMat4(const std::string& name, const glm::mat4& matrix) const;
};