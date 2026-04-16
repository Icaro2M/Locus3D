#pragma once

#include <vector>

class RadialSolidBuilder
{
public:
    struct Result
    {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
    };

public:
    static Result build(
        int sides,
        float height,
        float bottomRadius,
        float topRadius,
        bool capBottom,
        bool capTop
    );
};