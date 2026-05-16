#include "PrimitiveFactory.h"

#include "RadialSolidBuilder.h"
#include "UvSphereBuilder.h"

#include "geometry/LogicalFace.h"

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
        -hx, -hy, -hz, 0.0f, 0.0f, -1.0f,
         hx, -hy, -hz, 0.0f, 0.0f, -1.0f,
         hx,  hy, -hz, 0.0f, 0.0f, -1.0f,
        -hx,  hy, -hz, 0.0f, 0.0f, -1.0f,

        -hx, -hy,  hz, 0.0f, 0.0f, 1.0f,
         hx, -hy,  hz, 0.0f, 0.0f, 1.0f,
         hx,  hy,  hz, 0.0f, 0.0f, 1.0f,
        -hx,  hy,  hz, 0.0f, 0.0f, 1.0f,

        -hx, -hy, -hz, -1.0f, 0.0f, 0.0f,
        -hx,  hy, -hz, -1.0f, 0.0f, 0.0f,
        -hx,  hy,  hz, -1.0f, 0.0f, 0.0f,
        -hx, -hy,  hz, -1.0f, 0.0f, 0.0f,

         hx, -hy, -hz, 1.0f, 0.0f, 0.0f,
         hx,  hy, -hz, 1.0f, 0.0f, 0.0f,
         hx,  hy,  hz, 1.0f, 0.0f, 0.0f,
         hx, -hy,  hz, 1.0f, 0.0f, 0.0f,

        -hx, -hy, -hz, 0.0f, -1.0f, 0.0f,
        -hx, -hy,  hz, 0.0f, -1.0f, 0.0f,
         hx, -hy,  hz, 0.0f, -1.0f, 0.0f,
         hx, -hy, -hz, 0.0f, -1.0f, 0.0f,

        -hx,  hy, -hz, 0.0f, 1.0f, 0.0f,
        -hx,  hy,  hz, 0.0f, 1.0f, 0.0f,
         hx,  hy,  hz, 0.0f, 1.0f, 0.0f,
         hx,  hy, -hz, 0.0f, 1.0f, 0.0f
    };

    unsigned int indices[] = {
    0, 2, 1, 0, 3, 2,
    4, 5, 6, 4, 6, 7,
    8, 10, 9, 8, 11, 10,
    12, 13, 14, 12, 14, 15,
    16, 18, 17, 16, 19, 18,
    20, 21, 22, 20, 22, 23
    };

    Mesh mesh(vertices, sizeof(vertices), indices, 36);

    std::vector<LogicalFace> logicalFaces =
    {
        LogicalFace({ 0, 1 },   { 0, 1, 2, 3 }),
        LogicalFace({ 2, 3 },   { 4, 5, 6, 7 }),
        LogicalFace({ 4, 5 },   { 8, 9, 10, 11 }),
        LogicalFace({ 6, 7 },   { 12, 13, 14, 15 }),
        LogicalFace({ 8, 9 },   { 16, 17, 18, 19 }),
        LogicalFace({ 10, 11 }, { 20, 21, 22, 23 })
    };

    mesh.setLogicalFaces(logicalFaces);

    return mesh;
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

    Mesh mesh(
        data.vertices.data(),
        static_cast<unsigned int>(data.vertices.size() * sizeof(float)),
        data.indices.data(),
        static_cast<unsigned int>(data.indices.size())
    );

    mesh.setLogicalFaces(data.logicalFaces);

    return mesh;
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

    Mesh mesh(
        data.vertices.data(),
        static_cast<unsigned int>(data.vertices.size() * sizeof(float)),
        data.indices.data(),
        static_cast<unsigned int>(data.indices.size())
    );

    mesh.setLogicalFaces(data.logicalFaces);

    return mesh;
}