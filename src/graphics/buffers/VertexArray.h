#pragma once

#include <glad/glad.h>

class VertexArray
{
public:

    VertexArray();
    ~VertexArray();

    void bind() const;
    void unbind() const;

private:

    unsigned int arrayID;
};