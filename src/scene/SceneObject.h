#pragma once

#include <string>     // <-- Adicione esta linha para o std::string
#include <cstdint>    // <-- Adicione esta linha para o uint32_t

#include "../geometry/Mesh.h"
#include "../math/Transform.h"

class SceneObject
{
private:
    Mesh& m_Mesh;
    Transform m_Transform;

public:
    SceneObject(Mesh& mesh);

    Mesh& getMesh();
    const Mesh& getMesh() const;

    Transform& getTransform();
    const Transform& getTransform() const;

        // Adicione na parte public:
    uint32_t getId() const;
    std::string getName() const;
    void setName(const std::string& name);

    // Adicione na parte private:
    uint32_t m_id;
    std::string m_name;
    static uint32_t s_nextId;
};