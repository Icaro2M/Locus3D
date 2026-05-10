#pragma once

#include "../../math/Ray.h"
#include "../../geometry/Mesh.h"

#include <glm/glm/glm.hpp>

class RayMeshIntersection
{
public:
    static bool intersectRayTriangle(
        const Ray& ray,
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        float& outT
    );

    static bool intersectMesh(
        const Ray& ray,
        const Mesh& mesh,
        const glm::mat4& modelMatrix,
        float& outClosestT,
        int& outTriangleIndex
    );
};