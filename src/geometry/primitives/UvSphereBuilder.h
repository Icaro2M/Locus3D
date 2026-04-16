#pragma once

#include <vector>

class UvSphereBuilder
{
public:
    struct Result
    {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
    };

public:
    static Result build(
        int segments,
        int rings,
        float radiusX,
        float radiusY,
        float radiusZ
    );
};