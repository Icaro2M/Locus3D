#include "RotateGizmoRenderer.h"

#include <cmath>
#include <vector>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "../../resources/AssetPaths.h"

namespace
{
    constexpr float PI = 3.14159265358979323846f;
}

std::vector<float> RotateGizmoRenderer::buildColoredVertices(
    const std::vector<float>& basePositions,
    const glm::vec3& color
) const
{
    std::vector<float> result;
    result.reserve((basePositions.size() / 3) * 6);

    for (size_t i = 0; i < basePositions.size(); i += 3)
    {
        result.push_back(basePositions[i + 0]);
        result.push_back(basePositions[i + 1]);
        result.push_back(basePositions[i + 2]);

        result.push_back(color.r);
        result.push_back(color.g);
        result.push_back(color.b);
    }

    return result;
}

void RotateGizmoRenderer::buildRingBandMesh()
{
    m_RingBasePositions.clear();
    m_RingIndices.clear();

    const float innerRadius = m_RingRadius - (m_RingThickness * 0.5f);
    const float outerRadius = m_RingRadius + (m_RingThickness * 0.5f);

    for (int i = 0; i < m_RingSegments; ++i)
    {
        const float t = (2.0f * PI * static_cast<float>(i)) / static_cast<float>(m_RingSegments);
        const float c = std::cos(t);
        const float s = std::sin(t);

        m_RingBasePositions.push_back(innerRadius * c);
        m_RingBasePositions.push_back(innerRadius * s);
        m_RingBasePositions.push_back(0.0f);

        m_RingBasePositions.push_back(outerRadius * c);
        m_RingBasePositions.push_back(outerRadius * s);
        m_RingBasePositions.push_back(0.0f);
    }

    for (int i = 0; i < m_RingSegments; ++i)
    {
        const unsigned int currentInner = static_cast<unsigned int>(i * 2);
        const unsigned int currentOuter = static_cast<unsigned int>(i * 2 + 1);

        const int next = (i + 1) % m_RingSegments;
        const unsigned int nextInner = static_cast<unsigned int>(next * 2);
        const unsigned int nextOuter = static_cast<unsigned int>(next * 2 + 1);

        m_RingIndices.push_back(currentInner);
        m_RingIndices.push_back(currentOuter);
        m_RingIndices.push_back(nextOuter);

        m_RingIndices.push_back(currentInner);
        m_RingIndices.push_back(nextOuter);
        m_RingIndices.push_back(nextInner);

        m_RingIndices.push_back(currentInner);
        m_RingIndices.push_back(nextOuter);
        m_RingIndices.push_back(currentOuter);

        m_RingIndices.push_back(currentInner);
        m_RingIndices.push_back(nextInner);
        m_RingIndices.push_back(nextOuter);
    }
}

RotateGizmoRenderer::RotateGizmoRenderer()
    : m_Shader(
        AssetPaths::shader("helpers/transformGizmo/vertex.glsl"),
        AssetPaths::shader("helpers/transformGizmo/fragment.glsl")
    ),
    m_RingVBO(nullptr),
    m_RingEBO(nullptr),
    m_RingIndexCount(0),
    m_RingRadius(1.05f),
    m_RingThickness(0.025f),
    m_RingSegments(48)
{
    buildRingBandMesh();
    m_RingIndexCount = static_cast<unsigned int>(m_RingIndices.size());

    std::vector<float> initialVertices =
        buildColoredVertices(m_RingBasePositions, glm::vec3(1.0f, 0.0f, 0.0f));

    m_RingVAO.bind();

    m_RingVBO = new VertexBuffer(
        initialVertices.data(),
        static_cast<unsigned int>(initialVertices.size() * sizeof(float))
    );

    m_RingEBO = new IndexBuffer(
        m_RingIndices.data(),
        static_cast<unsigned int>(m_RingIndices.size())
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_RingVAO.unbind();
    m_RingVBO->unbind();
}

RotateGizmoRenderer::~RotateGizmoRenderer()
{
    delete m_RingVBO;
    delete m_RingEBO;
}

void RotateGizmoRenderer::render(
    const SceneObject& selectedObject,
    const Camera& camera,
    TransformAxis activeAxis,
    TransformSpace transformSpace
)
{
    glm::vec3 xColor(0.95f, 0.20f, 0.20f);
    glm::vec3 yColor(0.20f, 0.95f, 0.20f);
    glm::vec3 zColor(0.20f, 0.35f, 0.95f);

    glm::vec3 dimX(0.35f, 0.08f, 0.08f);
    glm::vec3 dimY(0.08f, 0.35f, 0.08f);
    glm::vec3 dimZ(0.08f, 0.12f, 0.35f);

    if (activeAxis == TransformAxis::X)
    {
        yColor = dimY;
        zColor = dimZ;
    }
    else if (activeAxis == TransformAxis::Y)
    {
        xColor = dimX;
        zColor = dimZ;
    }
    else if (activeAxis == TransformAxis::Z)
    {
        xColor = dimX;
        yColor = dimY;
    }

    glm::mat4 baseModel(1.0f);

    const glm::vec3& position = selectedObject.getTransform().getPosition();
    const glm::vec3& rotation = selectedObject.getTransform().getRotation();

    baseModel = glm::translate(baseModel, position);

    if (transformSpace == TransformSpace::Local)
    {
        baseModel = glm::rotate(baseModel, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        baseModel = glm::rotate(baseModel, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        baseModel = glm::rotate(baseModel, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

    glDisable(GL_DEPTH_TEST);

    glm::vec3 ringColors[3] = { xColor, yColor, zColor };

    glm::mat4 ringRotations[3] =
    {
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),  glm::vec3(0.0f, 1.0f, 0.0f)), 
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), 
        glm::mat4(1.0f)                                                                    
    };

    for (int i = 0; i < 3; ++i)
    {
        std::vector<float> ringVertices =
            buildColoredVertices(m_RingBasePositions, ringColors[i]);

        m_RingVBO->bind();
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<unsigned int>(ringVertices.size() * sizeof(float)),
            ringVertices.data()
        );

        glm::mat4 ringModel = baseModel * ringRotations[i];
        m_Shader.setMat4("u_Model", ringModel);

        m_RingVAO.bind();
        m_RingEBO->bind();
        glDrawElements(GL_TRIANGLES, m_RingIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    glEnable(GL_DEPTH_TEST);
}