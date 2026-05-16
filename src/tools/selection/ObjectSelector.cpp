#include "ObjectSelector.h"

#include "Raycaster.h"
#include "RayMeshIntersection.h"

#include "../../geometry/Mesh.h"

#include <limits>

SceneObject* ObjectSelector::selectObject(GLFWwindow* window, const Camera& camera, const Scene& scene)
{
    Raycaster raycaster;
    Ray ray = raycaster.buildRayFromMouse(window, camera);

    SceneObject* selectedObject = nullptr;
    float closestHit = std::numeric_limits<float>::max();

    for (SceneObject* object : scene.getObjects())
    {
        if (object == nullptr)
        {
            continue;
        }

        const Mesh& mesh = object->getMesh();
        glm::mat4 modelMatrix = object->getTransform().getModelMatrix();

        float meshHitT = 0.0f;
        int triangleIndex = -1;

        bool hit = RayMeshIntersection::intersectMesh(
            ray,
            mesh,
            modelMatrix,
            meshHitT,
            triangleIndex
        );

        if (hit && meshHitT < closestHit)
        {
            closestHit = meshHitT;
            selectedObject = object;
        }
    }

    return selectedObject;
}