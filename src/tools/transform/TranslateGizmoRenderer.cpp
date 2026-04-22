#include "TranslateGizmoRenderer.h"

#include <algorithm>
#include <limits>
#include <vector>

#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>

#include "../../resources/AssetPaths.h"
#include "../../geometry/Mesh.h"
#include "../../geometry/primitives/PrimitiveFactory.h"

std::vector<float> TranslateGizmoRenderer::buildColoredVertices(
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

void TranslateGizmoRenderer::extractPositionOnlyVertices(
    const std::vector<float>& interleavedVertices,
    std::vector<float>& outPositions
) const
{
    outPositions.clear();

    for (size_t i = 0; i < interleavedVertices.size(); i += 6)
    {
        outPositions.push_back(interleavedVertices[i + 0]);
        outPositions.push_back(interleavedVertices[i + 1]);
        outPositions.push_back(interleavedVertices[i + 2]);
    }
}

void TranslateGizmoRenderer::normalizeAxisPrimitiveToUnitPositiveY(std::vector<float>& positions) const
{
    if (positions.empty())
    {
        return;
    }

    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float minZ = std::numeric_limits<float>::max();

    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    float maxZ = std::numeric_limits<float>::lowest();

    for (size_t i = 0; i < positions.size(); i += 3)
    {
        const float x = positions[i + 0];
        const float y = positions[i + 1];
        const float z = positions[i + 2];

        minX = std::min(minX, x);
        minY = std::min(minY, y);
        minZ = std::min(minZ, z);

        maxX = std::max(maxX, x);
        maxY = std::max(maxY, y);
        maxZ = std::max(maxZ, z);
    }

    const float centerX = (minX + maxX) * 0.5f;
    const float centerZ = (minZ + maxZ) * 0.5f;

    const float height = maxY - minY;

    float maxRadius = 0.0f;
    for (size_t i = 0; i < positions.size(); i += 3)
    {
        const float localX = positions[i + 0] - centerX;
        const float localZ = positions[i + 2] - centerZ;
        const float radius = std::sqrt(localX * localX + localZ * localZ);
        maxRadius = std::max(maxRadius, radius);
    }

    if (height <= 0.00001f)
    {
        return;
    }

    if (maxRadius <= 0.00001f)
    {
        maxRadius = 1.0f;
    }

    for (size_t i = 0; i < positions.size(); i += 3)
    {
        positions[i + 0] = (positions[i + 0] - centerX) / maxRadius;
        positions[i + 1] = (positions[i + 1] - minY) / height;
        positions[i + 2] = (positions[i + 2] - centerZ) / maxRadius;
    }
}

TranslateGizmoRenderer::TranslateGizmoRenderer()
    : m_Shader(
        AssetPaths::shader("helpers/transformGizmo/vertex.glsl"),
        AssetPaths::shader("helpers/transformGizmo/fragment.glsl")
    ),
    m_ShaftVBO(nullptr),
    m_ShaftEBO(nullptr),
    m_ArrowVBO(nullptr),
    m_ArrowEBO(nullptr),
    m_CenterVBO(nullptr),
    m_CenterEBO(nullptr),
    m_ShaftIndexCount(0),
    m_ArrowIndexCount(0),
    m_CenterIndexCount(0),
    m_GizmoSize(1.10f),
    m_ShaftRadius(0.012f),
    m_ArrowLength(0.24f),
    m_ArrowRadius(0.080f),
    m_CenterSize(0.075f)
{

    Mesh shaftCylinder = PrimitiveFactory::createCylinder(1.0f, 1.0f, 10);

    extractPositionOnlyVertices(shaftCylinder.getVertices(), m_ShaftBasePositions);
    normalizeAxisPrimitiveToUnitPositiveY(m_ShaftBasePositions);

    m_ShaftIndices = shaftCylinder.getIndices();
    m_ShaftIndexCount = static_cast<unsigned int>(m_ShaftIndices.size());

    std::vector<float> initialShaftVertices =
        buildColoredVertices(m_ShaftBasePositions, glm::vec3(1.0f, 0.0f, 0.0f));

    m_ShaftVAO.bind();

    m_ShaftVBO = new VertexBuffer(
        initialShaftVertices.data(),
        static_cast<unsigned int>(initialShaftVertices.size() * sizeof(float))
    );

    m_ShaftEBO = new IndexBuffer(
        m_ShaftIndices.data(),
        static_cast<unsigned int>(m_ShaftIndices.size())
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_ShaftVAO.unbind();
    m_ShaftVBO->unbind();

    Mesh arrowCone = PrimitiveFactory::createCone(5, 1.0f, 10);

    extractPositionOnlyVertices(arrowCone.getVertices(), m_ArrowBasePositions);
    normalizeAxisPrimitiveToUnitPositiveY(m_ArrowBasePositions);

    m_ArrowIndices = arrowCone.getIndices();
    m_ArrowIndexCount = static_cast<unsigned int>(m_ArrowIndices.size());

    std::vector<float> initialArrowVertices =
        buildColoredVertices(m_ArrowBasePositions, glm::vec3(1.0f, 0.0f, 0.0f));

    m_ArrowVAO.bind();

    m_ArrowVBO = new VertexBuffer(
        initialArrowVertices.data(),
        static_cast<unsigned int>(initialArrowVertices.size() * sizeof(float))
    );

    m_ArrowEBO = new IndexBuffer(
        m_ArrowIndices.data(),
        static_cast<unsigned int>(m_ArrowIndices.size())
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_ArrowVAO.unbind();
    m_ArrowVBO->unbind();

    Mesh centerCube = PrimitiveFactory::createCube();

    extractPositionOnlyVertices(centerCube.getVertices(), m_CenterBasePositions);
    m_CenterIndices = centerCube.getIndices();
    m_CenterIndexCount = static_cast<unsigned int>(m_CenterIndices.size());

    std::vector<float> initialCenterVertices =
        buildColoredVertices(m_CenterBasePositions, glm::vec3(0.82f, 0.82f, 0.82f));

    m_CenterVAO.bind();

    m_CenterVBO = new VertexBuffer(
        initialCenterVertices.data(),
        static_cast<unsigned int>(initialCenterVertices.size() * sizeof(float))
    );

    m_CenterEBO = new IndexBuffer(
        m_CenterIndices.data(),
        static_cast<unsigned int>(m_CenterIndices.size())
    );

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    m_CenterVAO.unbind();
    m_CenterVBO->unbind();
}

TranslateGizmoRenderer::~TranslateGizmoRenderer()
{
    delete m_ShaftVBO;
    delete m_ShaftEBO;
    delete m_ArrowVBO;
    delete m_ArrowEBO;
    delete m_CenterVBO;
    delete m_CenterEBO;
}

void TranslateGizmoRenderer::render(
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

    const float shaftLength = m_GizmoSize - m_ArrowLength;

    glm::mat4 model(1.0f);

    const glm::vec3& position = selectedObject.getTransform().getPosition();
    const glm::vec3& rotation = selectedObject.getTransform().getRotation();

    model = glm::translate(model, position);

    if (transformSpace == TransformSpace::Local)
    {
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    }

    m_Shader.use();
    m_Shader.setMat4("u_View", camera.getViewMatrix());
    m_Shader.setMat4("u_Projection", camera.getProjectionMatrix());

    {
        std::vector<float> centerVertices =
            buildColoredVertices(m_CenterBasePositions, glm::vec3(0.85f, 0.85f, 0.85f));

        m_CenterVBO->bind();
        glBufferSubData(
            GL_ARRAY_BUFFER,
            0,
            static_cast<unsigned int>(centerVertices.size() * sizeof(float)),
            centerVertices.data()
        );

        glm::mat4 centerModel = glm::scale(model, glm::vec3(m_CenterSize));
        m_Shader.setMat4("u_Model", centerModel);

        m_CenterVAO.bind();
        m_CenterEBO->bind();
        glDrawElements(GL_TRIANGLES, m_CenterIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    glm::vec3 axisColors[3] = { xColor, yColor, zColor };

    glm::mat4 axisRotations[3] =
    {
        glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::mat4(1.0f),                                                                
        glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f))  
    };

    glm::vec3 arrowPositions[3] =
    {
        glm::vec3(shaftLength, 0.0f, 0.0f),
        glm::vec3(0.0f, shaftLength, 0.0f),
        glm::vec3(0.0f, 0.0f, shaftLength)
    };

    for (int i = 0; i < 3; ++i)
    {
        {
            std::vector<float> shaftVertices =
                buildColoredVertices(m_ShaftBasePositions, axisColors[i]);

            m_ShaftVBO->bind();
            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                static_cast<unsigned int>(shaftVertices.size() * sizeof(float)),
                shaftVertices.data()
            );

            glm::mat4 shaftModel = model;
            shaftModel = shaftModel * axisRotations[i];
            shaftModel = glm::scale(shaftModel, glm::vec3(m_ShaftRadius, shaftLength, m_ShaftRadius));

            m_Shader.setMat4("u_Model", shaftModel);

            m_ShaftVAO.bind();
            m_ShaftEBO->bind();
            glDrawElements(GL_TRIANGLES, m_ShaftIndexCount, GL_UNSIGNED_INT, nullptr);
        }

        {
            std::vector<float> arrowVertices =
                buildColoredVertices(m_ArrowBasePositions, axisColors[i]);

            m_ArrowVBO->bind();
            glBufferSubData(
                GL_ARRAY_BUFFER,
                0,
                static_cast<unsigned int>(arrowVertices.size() * sizeof(float)),
                arrowVertices.data()
            );

            glm::mat4 arrowModel = model;
            arrowModel = glm::translate(arrowModel, arrowPositions[i]);
            arrowModel = arrowModel * axisRotations[i];
            arrowModel = glm::scale(arrowModel, glm::vec3(m_ArrowRadius, m_ArrowLength, m_ArrowRadius));

            m_Shader.setMat4("u_Model", arrowModel);

            m_ArrowVAO.bind();
            m_ArrowEBO->bind();
            glDrawElements(GL_TRIANGLES, m_ArrowIndexCount, GL_UNSIGNED_INT, nullptr);
        }
    }
}