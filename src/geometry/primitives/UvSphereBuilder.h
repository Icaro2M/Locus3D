#pragma once

#include <vector>

#include "geometry/LogicalFace.h"

class UvSphereBuilder
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
        int segments,
        int rings,
        float radiusX,
        float radiusY,
        float radiusZ
    );
};