#pragma once

#include <vector>

#include <glm/glm/glm.hpp>

#include "../../graphics/Shader.h"
#include "../../graphics/buffers/VertexArray.h"
#include "../../graphics/buffers/VertexBuffer.h"
#include "../../graphics/buffers/IndexBuffer.h"
#include "../../scene/SceneObject.h"
#include "../../scene/Camera.h"
#include "TransformController.h"

class RotateGizmoRenderer
{
private:
    Shader m_Shader;

    VertexArray m_RingVAO;
    VertexBuffer* m_RingVBO;
    IndexBuffer* m_RingEBO;

    std::vector<float> m_RingBasePositions;
    std::vector<unsigned int> m_RingIndices;

    unsigned int m_RingIndexCount;

    float m_RingRadius;
    float m_RingThickness;
    int m_RingSegments;

private:
    std::vector<float> buildColoredVertices(
        const std::vector<float>& basePositions,
        const glm::vec3& color
    ) const;

    void buildRingBandMesh();

public:
    RotateGizmoRenderer();
    ~RotateGizmoRenderer();

    void render(
        const SceneObject& selectedObject,
        const Camera& camera,
        TransformAxis activeAxis,
        TransformSpace transformSpace
    );
};