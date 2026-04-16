#pragma once

#include "../Mesh.h"

class PrimitiveFactory
{
public:
    static Mesh createCube();
    static Mesh createBox(float width, float height, float depth);
    static Mesh createTetrahedron();

    static Mesh createRadialSolid(
        int sides,
        float height,
        float bottomRadius,
        float topRadius,
        bool capBottom,
        bool capTop
    );

    static Mesh createCylinder(
        int sides,
        float radius,
        float height
    );

    static Mesh createCone(
        int sides,
        float radius,
        float height
    );

    static Mesh createPrism(
        int sides,
        float radius,
        float height
    );

    static Mesh createUvSphere(
        int segments,
        int rings,
        float radius
    );

    static Mesh createEllipsoid(
        int segments,
        int rings,
        float radiusX,
        float radiusY,
        float radiusZ
    );
};