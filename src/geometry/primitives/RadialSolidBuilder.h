#pragma once

#include <vector>

#include "geometry/LogicalFace.h"

class RadialSolidBuilder
{
public:
    struct Result
    {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;
        std::vector<LogicalFace> logicalFaces;
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