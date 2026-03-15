#include "PrimitiveFactory.h"

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
    float vertices[] =
    {

         0.0f,  0.6f,  0.0f,   0.0f,  0.4472f,  0.8944f,
        -0.5f, -0.3f,  0.5f,   0.0f,  0.4472f,  0.8944f,
         0.5f, -0.3f,  0.5f,   0.0f,  0.4472f,  0.8944f,


          0.0f,  0.6f,  0.0f,   0.8944f,  0.4472f,  0.0f,
          0.5f, -0.3f,  0.5f,   0.8944f,  0.4472f,  0.0f,
          0.5f, -0.3f, -0.5f,   0.8944f,  0.4472f,  0.0f,


           0.0f,  0.6f,  0.0f,   0.0f,  0.4472f, -0.8944f,
           0.5f, -0.3f, -0.5f,   0.0f,  0.4472f, -0.8944f,
          -0.5f, -0.3f, -0.5f,   0.0f,  0.4472f, -0.8944f,


           0.0f,  0.6f,  0.0f,  -0.8944f,  0.4472f,  0.0f,
          -0.5f, -0.3f, -0.5f,  -0.8944f,  0.4472f,  0.0f,
          -0.5f, -0.3f,  0.5f,  -0.8944f,  0.4472f,  0.0f
    };

    unsigned int indices[] =
    {
        0, 1, 2,
        3, 4, 5,
        6, 7, 8,
        9, 10, 11
    };

    return Mesh(vertices, sizeof(vertices), indices, 12);
}