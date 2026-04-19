#include "VertexArray.h"

VertexArray::VertexArray()
    : arrayID(0)
{
    glGenVertexArrays(1, &arrayID);
}

VertexArray::~VertexArray()
{
    if (arrayID != 0)
    {
        glDeleteVertexArrays(1, &arrayID);
    }
}

VertexArray::VertexArray(VertexArray&& other) noexcept
    : arrayID(other.arrayID)
{
    other.arrayID = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& other) noexcept
{
    if (this != &other)
    {
        if (arrayID != 0)
        {
            glDeleteVertexArrays(1, &arrayID);
        }

        arrayID = other.arrayID;
        other.arrayID = 0;
    }

    return *this;
}

void VertexArray::bind() const
{
    glBindVertexArray(arrayID);
}

void VertexArray::unbind() const
{
    glBindVertexArray(0);
}