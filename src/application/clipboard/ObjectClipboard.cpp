#include "ObjectClipboard.h"

void ObjectClipboard::copyFrom(const SceneObject& object)
{
    const Mesh& mesh = object.getMesh();
    const Transform& transform = object.getTransform();

    m_name = object.getName();
    m_vertices = mesh.getVertices();
    m_indices = mesh.getIndices();
    m_logicalFaces = mesh.getLogicalFaces();

    m_position = transform.getPosition();
    m_rotation = transform.getRotation();
    m_scale = transform.getScale();

    m_hasData = true;
    m_pasteCount = 0;
}

SceneObject* ObjectClipboard::pasteInto(SceneContext& sceneContext)
{
    if (!m_hasData || m_vertices.empty() || m_indices.empty())
    {
        return nullptr;
    }

    m_pasteCount++;

    glm::vec3 pastePosition = m_position + glm::vec3(
        0.35f * static_cast<float>(m_pasteCount),
        0.0f,
        0.35f * static_cast<float>(m_pasteCount)
    );

    std::string pastedName = m_name + " Cópia";

    return sceneContext.createObjectFromMeshData(
        pastedName,
        m_vertices,
        m_indices,
        m_logicalFaces,
        pastePosition,
        m_rotation,
        m_scale
    );
}

bool ObjectClipboard::hasData() const
{
    return m_hasData;
}

void ObjectClipboard::clear()
{
    m_name.clear();
    m_vertices.clear();
    m_indices.clear();
    m_logicalFaces.clear();

    m_position = glm::vec3(0.0f);
    m_rotation = glm::vec3(0.0f);
    m_scale = glm::vec3(1.0f);

    m_hasData = false;
    m_pasteCount = 0;
}