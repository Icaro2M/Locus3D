#pragma once

#include "../../scene/SceneObject.h"

#include <glm/glm/glm.hpp>

#include <vector>

class FaceGeometry
{
private:
    SceneObject* m_Object;
    int m_FaceIndex;

    std::vector<unsigned int> m_TriangleIndices;
    std::vector<unsigned int> m_BoundaryVertexIndices;
    std::vector<glm::vec3> m_LocalBoundaryVertices;

    glm::vec3 m_LocalNormal;
    glm::vec3 m_LocalCenter;

public:
    FaceGeometry();

    void setObject(SceneObject* object);
    void setFaceIndex(int faceIndex);

    void setTriangleIndices(const std::vector<unsigned int>& triangleIndices);
    void setBoundaryVertexIndices(const std::vector<unsigned int>& boundaryVertexIndices);
    void setLocalBoundaryVertices(const std::vector<glm::vec3>& localBoundaryVertices);

    void setLocalNormal(const glm::vec3& normal);
    void setLocalCenter(const glm::vec3& center);

    SceneObject* getObject() const;
    int getFaceIndex() const;

    const std::vector<unsigned int>& getTriangleIndices() const;
    const std::vector<unsigned int>& getBoundaryVertexIndices() const;
    const std::vector<glm::vec3>& getLocalBoundaryVertices() const;

    const glm::vec3& getLocalNormal() const;
    const glm::vec3& getLocalCenter() const;

    bool isValid() const;
    void clear();
};