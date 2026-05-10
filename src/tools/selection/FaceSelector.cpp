#include "FaceSelector.h"

#include "Raycaster.h"
#include "RayMeshIntersection.h"

#include "../../geometry/Mesh.h"

int FaceSelector::selectFace(
    const SceneObject& selectedObject,
    GLFWwindow* window,
    const Camera& camera
) const
{
    Raycaster raycaster;
    Ray ray = raycaster.buildRayFromMouse(window, camera);

    const Mesh& mesh = selectedObject.getMesh();
    glm::mat4 modelMatrix = selectedObject.getTransform().getModelMatrix();

    float closestT = 0.0f;
    int selectedTriangleIndex = -1;

    bool hit = RayMeshIntersection::intersectMesh(
        ray,
        mesh,
        modelMatrix,
        closestT,
        selectedTriangleIndex
    );

    if (!hit)
    {
        return -1;
    }

    if (!mesh.hasLogicalFaces())
    {
        return selectedTriangleIndex;
    }

    return mesh.getLogicalFaceIndexFromTriangle(static_cast<unsigned int>(selectedTriangleIndex));
}