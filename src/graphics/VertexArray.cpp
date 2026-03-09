#include "VertexArray.h"

VertexArray::VertexArray()
{
    glGenVertexArrays(1, &arrayID);
}

VertexArray::~VertexArray()
{
    glDeleteVertexArrays(1, &arrayID);
}

void VertexArray::bind() const
{
    glBindVertexArray(arrayID);
}

void VertexArray::unbind() const
{
    glBindVertexArray(0);
}