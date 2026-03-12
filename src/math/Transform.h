#pragma once

#include <glm/glm/glm.hpp>

class Transform
{
private:
    glm::vec3 m_Position;
    glm::vec3 m_Rotation;
    glm::vec3 m_Scale;

public:
    Transform();

    Transform(const glm::vec3& position,
        const glm::vec3& rotation,
        const glm::vec3& scale);

    void setPosition(const glm::vec3& position);
    void setRotation(const glm::vec3& rotation);
    void setScale(const glm::vec3& scale);

    const glm::vec3& getPosition() const;
    const glm::vec3& getRotation() const;
    const glm::vec3& getScale() const;

    glm::mat4 getModelMatrix() const;
};