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

class TranslateGizmoRenderer
{
private:
    Shader m_Shader;

    VertexArray m_ShaftVAO;
    VertexBuffer* m_ShaftVBO;
    IndexBuffer* m_ShaftEBO;

    VertexArray m_ArrowVAO;
    VertexBuffer* m_ArrowVBO;
    IndexBuffer* m_ArrowEBO;

    VertexArray m_CenterVAO;
    VertexBuffer* m_CenterVBO;
    IndexBuffer* m_CenterEBO;

    std::vector<float> m_ShaftBasePositions;
    std::vector<unsigned int> m_ShaftIndices;

    std::vector<float> m_ArrowBasePositions;
    std::vector<unsigned int> m_ArrowIndices;

    std::vector<float> m_CenterBasePositions;
    std::vector<unsigned int> m_CenterIndices;

    unsigned int m_ShaftIndexCount;
    unsigned int m_ArrowIndexCount;
    unsigned int m_CenterIndexCount;

    float m_GizmoSize;
    float m_ShaftRadius;
    float m_ArrowLength;
    float m_ArrowRadius;
    float m_CenterSize;

private:
    std::vector<float> buildColoredVertices(
        const std::vector<float>& basePositions,
        const glm::vec3& color
    ) const;

    void extractPositionOnlyVertices(
        const std::vector<float>& interleavedVertices,
        std::vector<float>& outPositions
    ) const;

    void normalizeAxisPrimitiveToUnitPositiveY(std::vector<float>& positions) const;

public:
    TranslateGizmoRenderer();
    ~TranslateGizmoRenderer();

    void render(
        const SceneObject& selectedObject,
        const Camera& camera,
        TransformAxis activeAxis,
        TransformSpace transformSpace
    );
};