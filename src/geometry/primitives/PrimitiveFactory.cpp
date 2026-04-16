#include "PrimitiveFactory.h"
#include "RadialSolidBuilder.h"
#include "UvSphereBuilder.h"

Mesh PrimitiveFactory::createCube()
{
    return createBox(1.0f, 1.0f, 1.0f);
}

Mesh PrimitiveFactory::createBox(float width, float height, float depth)
{
    float hx = width / 2.0f;
    float hy = height / 2.0f;
    float hz = depth / 2.0f;

    float vertices[] =
    {
        -hx, -hy, -hz,  0.0f,  0.0f, -1.0f,
         hx, -hy, -hz,  0.0f,  0.0f, -1.0f,
         hx,  hy, -hz,  0.0f,  0.0f, -1.0f,
        -hx,  hy, -hz,  0.0f,  0.0f, -1.0f,

        -hx, -hy,  hz,  0.0f,  0.0f,  1.0f,
         hx, -hy,  hz,  0.0f,  0.0f,  1.0f,
         hx,  hy,  hz,  0.0f,  0.0f,  1.0f,
        -hx,  hy,  hz,  0.0f,  0.0f,  1.0f,

        -hx, -hy, -hz, -1.0f,  0.0f,  0.0f,
        -hx,  hy, -hz, -1.0f,  0.0f,  0.0f,
        -hx,  hy,  hz, -1.0f,  0.0f,  0.0f,
        -hx, -hy,  hz, -1.0f,  0.0f,  0.0f,

         hx, -hy, -hz,  1.0f,  0.0f,  0.0f,
         hx,  hy, -hz,  1.0f,  0.0f,  0.0f,
         hx,  hy,  hz,  1.0f,  0.0f,  0.0f,
         hx, -hy,  hz,  1.0f,  0.0f,  0.0f,

        -hx, -hy, -hz,  0.0f, -1.0f,  0.0f,
        -hx, -hy,  hz,  0.0f, -1.0f,  0.0f,
         hx, -hy,  hz,  0.0f, -1.0f,  0.0f,
         hx, -hy, -hz,  0.0f, -1.0f,  0.0f,

        -hx,  hy, -hz,  0.0f,  1.0f,  0.0f,
        -hx,  hy,  hz,  0.0f,  1.0f,  0.0f,
         hx,  hy,  hz,  0.0f,  1.0f,  0.0f,
         hx,  hy, -hz,  0.0f,  1.0f,  0.0f
    };

    unsigned int indices[] =
    {
         0,  1,  2,   0,  2,  3,
         4,  5,  6,   4,  6,  7,
         8,  9, 10,   8, 10, 11,
        12, 13, 14,  12, 14, 15,
        16, 17, 18,  16, 18, 19,
        20, 21, 22,  20, 22, 23
    };

    return Mesh(vertices, sizeof(vertices), indices, 36);
}

Mesh PrimitiveFactory::createTetrahedron()
{
    return createRadialSolid(3, 1.2f, 0.7f, 0.0f, true, false);
}

Mesh PrimitiveFactory::createRadialSolid(
    int sides,
    float height,
    float bottomRadius,
    float topRadius,
    bool capBottom,
    bool capTop
)
{
    RadialSolidBuilder::Result data = RadialSolidBuilder::build(
        sides,
        height,
        bottomRadius,
        topRadius,
        capBottom,
        capTop
    );

    return Mesh(
        data.vertices.data(),
        static_cast<unsigned int>(data.vertices.size() * sizeof(float)),
        data.indices.data(),
        static_cast<unsigned int>(data.indices.size())
    );
}

Mesh PrimitiveFactory::createCylinder(
    int sides,
    float radius,
    float height
)
{
    return createRadialSolid(sides, height, radius, radius, true, true);
}

Mesh PrimitiveFactory::createCone(
    int sides,
    float radius,
    float height
)
{
    return createRadialSolid(sides, height, radius, 0.0f, true, false);
}

Mesh PrimitiveFactory::createPrism(
    int sides,
    float radius,
    float height
)
{
    return createRadialSolid(sides, height, radius, radius, true, true);
}

Mesh PrimitiveFactory::createUvSphere(
    int segments,
    int rings,
    float radius
)
{
    return createEllipsoid(segments, rings, radius, radius, radius);
}

Mesh PrimitiveFactory::createEllipsoid(
    int segments,
    int rings,
    float radiusX,
    float radiusY,
    float radiusZ
)
{
    UvSphereBuilder::Result data = UvSphereBuilder::build(
        segments,
        rings,
        radiusX,
        radiusY,
        radiusZ
    );

    return Mesh(
        data.vertices.data(),
        static_cast<unsigned int>(data.vertices.size() * sizeof(float)),
        data.indices.data(),
        static_cast<unsigned int>(data.indices.size())
    );
}