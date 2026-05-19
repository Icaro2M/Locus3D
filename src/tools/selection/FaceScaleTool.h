#pragma once

#include <vector>
#include <string>

#include <glm/glm/glm.hpp>

struct GLFWwindow;

#include "../../scene/Camera.h"

#include "FaceSelection.h"
#include "FaceGeometry.h"
#include "Raycaster.h"

class FaceScaleTool
{
private:
    bool m_Active;

    FaceSelection m_Selection;
    FaceGeometry m_BaseGeometry;

    std::vector<unsigned int> m_CoincidentBoundaryVertexIndices;

    float m_CurrentScaleFactor;

    std::string m_InputBuffer;
    bool m_UsingNumericInput;
    bool m_HasCommittedNumericValue;

    Raycaster m_Raycaster;

    glm::vec3 m_WorldCenter;
    glm::vec3 m_PlaneNormalWorld;
    float m_StartDistanceToCenter;

private:
    void buildCoincidentBoundaryVertexSet();

    bool intersectRayWithFacePlane(
        const Ray& ray,
        glm::vec3& intersectionPoint
    ) const;

public:
    FaceScaleTool();

    bool start(const FaceSelection& selection, GLFWwindow* window, const Camera& camera);
    void update(GLFWwindow* window, const Camera& camera);
    void onKeyPressed(int key);

    bool confirm();
    void cancel();

    bool isActive() const;
    float getCurrentScaleFactor() const;
    void setCurrentScaleFactorFromNumericInput(float scaleFactor);

    const FaceSelection& getSelection() const;
    const FaceGeometry& getBaseGeometry() const;
    const std::vector<unsigned int>& getCoincidentBoundaryVertexIndices() const;
};
