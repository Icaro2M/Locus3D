#include "FaceScaleTool.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>

#include <glm/glm/glm.hpp>

#include "../../geometry/Mesh.h"
#include "../../geometry/LogicalFace.h"
#include "../../scene/SceneObject.h"
#include "../../math/Transform.h"
#include "FaceGeometryBuilder.h"

namespace
{
    bool containsIndex(
        const std::vector<unsigned int>& values,
        unsigned int target
    )
    {
        for (unsigned int value : values)
        {
            if (value == target)
            {
                return true;
            }
        }

        return false;
    }

    bool isSamePosition(
        const glm::vec3& a,
        const glm::vec3& b,
        float epsilon = 0.0001f
    )
    {
        return glm::length(a - b) <= epsilon;
    }

    float clampScaleFactor(float value)
    {
        return std::max(0.05f, value);
    }
}

FaceScaleTool::FaceScaleTool()
    : m_Active(false),
    m_CurrentScaleFactor(1.0f),
    m_InputBuffer(""),
    m_UsingNumericInput(false),
    m_HasCommittedNumericValue(false),
    m_WorldCenter(0.0f, 0.0f, 0.0f),
    m_PlaneNormalWorld(0.0f, 1.0f, 0.0f),
    m_StartDistanceToCenter(1.0f)
{
}

bool FaceScaleTool::start(
    const FaceSelection& selection,
    GLFWwindow* window,
    const Camera& camera
)
{
    if (!selection.isValid())
    {
        return false;
    }

    SceneObject* object = selection.getObject();
    if (object == nullptr)
    {
        return false;
    }

    m_Selection = selection;

    FaceGeometryBuilder builder;
    m_BaseGeometry = builder.build(m_Selection);

    if (!m_BaseGeometry.isValid())
    {
        m_Selection.clear();
        return false;
    }

    glm::vec3 localCenter = m_BaseGeometry.getLocalCenter();
    glm::vec3 localNormal = m_BaseGeometry.getLocalNormal();

    if (glm::length(localNormal) <= 0.00001f)
    {
        m_Selection.clear();
        m_BaseGeometry.clear();
        return false;
    }

    localNormal = glm::normalize(localNormal);

    const glm::mat4 modelMatrix = object->getTransform().getModelMatrix();

    m_WorldCenter =
        glm::vec3(modelMatrix * glm::vec4(localCenter, 1.0f));

    m_PlaneNormalWorld = glm::mat3(modelMatrix) * localNormal;

    if (glm::length(m_PlaneNormalWorld) <= 0.00001f)
    {
        m_Selection.clear();
        m_BaseGeometry.clear();
        return false;
    }

    m_PlaneNormalWorld = glm::normalize(m_PlaneNormalWorld);

    const Ray initialRay = m_Raycaster.buildRayFromMouse(window, camera);

    glm::vec3 initialIntersectionPoint(0.0f, 0.0f, 0.0f);
    if (!intersectRayWithFacePlane(initialRay, initialIntersectionPoint))
    {
        m_Selection.clear();
        m_BaseGeometry.clear();
        return false;
    }

    m_StartDistanceToCenter =
        glm::length(initialIntersectionPoint - m_WorldCenter);

    if (m_StartDistanceToCenter < 0.0001f)
    {
        m_StartDistanceToCenter = 0.0001f;
    }

    buildCoincidentBoundaryVertexSet();

    if (m_CoincidentBoundaryVertexIndices.empty())
    {
        m_Selection.clear();
        m_BaseGeometry.clear();
        return false;
    }

    m_CurrentScaleFactor = 1.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_Active = true;

    return true;
}

void FaceScaleTool::update(GLFWwindow* window, const Camera& camera)
{
    if (!m_Active)
    {
        return;
    }

    if (m_UsingNumericInput || m_HasCommittedNumericValue)
    {
        return;
    }

    const Ray currentRay = m_Raycaster.buildRayFromMouse(window, camera);

    glm::vec3 currentIntersectionPoint(0.0f, 0.0f, 0.0f);
    if (!intersectRayWithFacePlane(currentRay, currentIntersectionPoint))
    {
        return;
    }

    const float currentDistance =
        glm::length(currentIntersectionPoint - m_WorldCenter);

    if (m_StartDistanceToCenter < 0.0001f)
    {
        m_CurrentScaleFactor = 1.0f;
        return;
    }

    m_CurrentScaleFactor =
        clampScaleFactor(currentDistance / m_StartDistanceToCenter);
}

void FaceScaleTool::onKeyPressed(int key)
{
    if (!m_Active)
    {
        return;
    }

    if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9)
    {
        m_InputBuffer += static_cast<char>('0' + (key - GLFW_KEY_0));
        m_UsingNumericInput = true;
        m_HasCommittedNumericValue = false;
        return;
    }

    if (key == GLFW_KEY_PERIOD || key == GLFW_KEY_KP_DECIMAL)
    {
        if (m_InputBuffer.find('.') == std::string::npos)
        {
            if (m_InputBuffer.empty())
            {
                m_InputBuffer += "0";
            }

            m_InputBuffer += '.';
            m_UsingNumericInput = true;
            m_HasCommittedNumericValue = false;
        }

        return;
    }

    if (key == GLFW_KEY_BACKSPACE)
    {
        if (!m_InputBuffer.empty())
        {
            m_InputBuffer.pop_back();

            if (m_InputBuffer.empty())
            {
                m_UsingNumericInput = false;
            }
        }

        return;
    }

    if (key == GLFW_KEY_ENTER || key == GLFW_KEY_KP_ENTER)
    {
        if (!m_InputBuffer.empty() && m_InputBuffer != ".")
        {
            try
            {
                m_CurrentScaleFactor = clampScaleFactor(std::stof(m_InputBuffer));
                m_HasCommittedNumericValue = true;
            }
            catch (...)
            {
            }
        }

        m_InputBuffer.clear();
        m_UsingNumericInput = false;
        return;
    }
}

bool FaceScaleTool::confirm()
{
    if (!m_Active || !m_Selection.isValid())
    {
        return false;
    }

    SceneObject* object = m_Selection.getObject();
    if (object == nullptr)
    {
        cancel();
        return false;
    }

    Mesh& mesh = object->getMesh();

    std::vector<float> vertices = mesh.getVertices();
    const std::vector<unsigned int> indices = mesh.getIndices();
    const std::vector<LogicalFace> logicalFaces = mesh.getLogicalFaces();

    const glm::vec3 center = m_BaseGeometry.getLocalCenter();

    for (unsigned int vertexIndex : m_CoincidentBoundaryVertexIndices)
    {
        const unsigned int base = vertexIndex * 6;

        if (base + 2 >= vertices.size())
        {
            continue;
        }

        const glm::vec3 originalPosition(
            vertices[base + 0],
            vertices[base + 1],
            vertices[base + 2]
        );

        const glm::vec3 offset = originalPosition - center;
        const glm::vec3 scaledPosition = center + offset * m_CurrentScaleFactor;

        vertices[base + 0] = scaledPosition.x;
        vertices[base + 1] = scaledPosition.y;
        vertices[base + 2] = scaledPosition.z;
    }

    mesh.updateGeometry(vertices, indices);

    if (!logicalFaces.empty())
    {
        mesh.setLogicalFaces(logicalFaces);
    }

    cancel();
    return true;
}

void FaceScaleTool::cancel()
{
    m_Active = false;
    m_Selection.clear();
    m_BaseGeometry.clear();
    m_CoincidentBoundaryVertexIndices.clear();
    m_CurrentScaleFactor = 1.0f;
    m_InputBuffer.clear();
    m_UsingNumericInput = false;
    m_HasCommittedNumericValue = false;
    m_WorldCenter = glm::vec3(0.0f, 0.0f, 0.0f);
    m_PlaneNormalWorld = glm::vec3(0.0f, 1.0f, 0.0f);
    m_StartDistanceToCenter = 1.0f;
}

bool FaceScaleTool::isActive() const
{
    return m_Active;
}

float FaceScaleTool::getCurrentScaleFactor() const
{
    return m_CurrentScaleFactor;
}

const FaceSelection& FaceScaleTool::getSelection() const
{
    return m_Selection;
}

const FaceGeometry& FaceScaleTool::getBaseGeometry() const
{
    return m_BaseGeometry;
}

const std::vector<unsigned int>& FaceScaleTool::getCoincidentBoundaryVertexIndices() const
{
    return m_CoincidentBoundaryVertexIndices;
}

void FaceScaleTool::buildCoincidentBoundaryVertexSet()
{
    m_CoincidentBoundaryVertexIndices.clear();

    if (!m_BaseGeometry.isValid())
    {
        return;
    }

    SceneObject* object = m_BaseGeometry.getObject();
    if (object == nullptr)
    {
        return;
    }

    const Mesh& mesh = object->getMesh();
    const std::vector<float>& vertices = mesh.getVertices();
    const std::vector<unsigned int>& boundaryIndices =
        m_BaseGeometry.getBoundaryVertexIndices();

    if (boundaryIndices.empty())
    {
        return;
    }

    std::vector<glm::vec3> boundaryPositions;
    boundaryPositions.reserve(boundaryIndices.size());

    for (unsigned int boundaryVertexIndex : boundaryIndices)
    {
        boundaryPositions.push_back(mesh.getVertexPosition(boundaryVertexIndex));
    }

    const unsigned int vertexCount =
        static_cast<unsigned int>(vertices.size() / 6);

    for (unsigned int candidateIndex = 0; candidateIndex < vertexCount; ++candidateIndex)
    {
        const glm::vec3 candidatePosition =
            mesh.getVertexPosition(candidateIndex);

        for (const glm::vec3& boundaryPosition : boundaryPositions)
        {
            if (isSamePosition(candidatePosition, boundaryPosition))
            {
                if (!containsIndex(m_CoincidentBoundaryVertexIndices, candidateIndex))
                {
                    m_CoincidentBoundaryVertexIndices.push_back(candidateIndex);
                }

                break;
            }
        }
    }
}

bool FaceScaleTool::intersectRayWithFacePlane(
    const Ray& ray,
    glm::vec3& intersectionPoint
) const
{
    const float denominator = glm::dot(ray.direction, m_PlaneNormalWorld);

    if (std::fabs(denominator) < 0.00001f)
    {
        return false;
    }

    const float t =
        glm::dot(m_WorldCenter - ray.origin, m_PlaneNormalWorld) / denominator;

    if (t < 0.0f)
    {
        return false;
    }

    intersectionPoint = ray.origin + ray.direction * t;
    return true;
}