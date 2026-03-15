#pragma once

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
};