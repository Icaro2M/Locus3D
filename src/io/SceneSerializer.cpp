#include "io/SceneSerializer.h"

#include "geometry/LogicalFace.h"

#include <fstream>
#include <memory>
#include <vector>

#include "json.hpp"

using json = nlohmann::json;

namespace
{
    json vec3ToJson(const glm::vec3& value)
    {
        return json::array({ value.x, value.y, value.z });
    }

    glm::vec3 jsonToVec3(const json& value)
    {
        return glm::vec3(
            value.at(0).get<float>(),
            value.at(1).get<float>(),
            value.at(2).get<float>()
        );
    }

    json logicalFaceToJson(const LogicalFace& face)
    {
        json faceJson;

        faceJson["triangleIndices"] = face.getTriangleIndices();
        faceJson["boundaryVertexIndices"] = face.getBoundaryVertexIndices();

        return faceJson;
    }

    LogicalFace jsonToLogicalFace(const json& faceJson)
    {
        std::vector<unsigned int> triangleIndices =
            faceJson.at("triangleIndices").get<std::vector<unsigned int>>();

        std::vector<unsigned int> boundaryVertexIndices =
            faceJson.at("boundaryVertexIndices").get<std::vector<unsigned int>>();

        return LogicalFace(triangleIndices, boundaryVertexIndices);
    }
}

bool SceneSerializer::save(const Scene& scene, const std::string& filePath)
{
    json root;
    root["version"] = 1;
    root["objects"] = json::array();

    const std::vector<SceneObject*>& objects = scene.getObjects();

    for (const SceneObject* object : objects)
    {
        if (object == nullptr)
        {
            continue;
        }

        const Mesh& mesh = object->getMesh();
        const Transform& transform = object->getTransform();

        json objectJson;

        objectJson["transform"]["position"] = vec3ToJson(transform.getPosition());
        objectJson["transform"]["rotation"] = vec3ToJson(transform.getRotation());
        objectJson["transform"]["scale"] = vec3ToJson(transform.getScale());

        objectJson["mesh"]["vertices"] = mesh.getVertices();
        objectJson["mesh"]["indices"] = mesh.getIndices();

        objectJson["mesh"]["logicalFaces"] = json::array();

        for (const LogicalFace& face : mesh.getLogicalFaces())
        {
            objectJson["mesh"]["logicalFaces"].push_back(logicalFaceToJson(face));
        }

        root["objects"].push_back(objectJson);
    }

    std::ofstream output(filePath);

    if (!output.is_open())
    {
        return false;
    }

    output << root.dump(4);

    return true;
}

SceneSaveData SceneSerializer::load(const std::string& filePath)
{
    SceneSaveData saveData;

    std::ifstream input(filePath);

    if (!input.is_open())
    {
        return saveData;
    }

    json root;
    input >> root;

    if (!root.contains("objects"))
    {
        return saveData;
    }

    for (const json& objectJson : root.at("objects"))
    {
        std::vector<float> vertices =
            objectJson.at("mesh").at("vertices").get<std::vector<float>>();

        std::vector<unsigned int> indices =
            objectJson.at("mesh").at("indices").get<std::vector<unsigned int>>();

        auto mesh = std::make_unique<Mesh>(
            vertices.data(),
            static_cast<unsigned int>(vertices.size() * sizeof(float)),
            indices.data(),
            static_cast<unsigned int>(indices.size())
        );

        std::vector<LogicalFace> logicalFaces;

        if (objectJson.at("mesh").contains("logicalFaces"))
        {
            for (const json& faceJson : objectJson.at("mesh").at("logicalFaces"))
            {
                logicalFaces.push_back(jsonToLogicalFace(faceJson));
            }
        }

        mesh->setLogicalFaces(logicalFaces);

        auto object = std::make_unique<SceneObject>(*mesh);

        const json& transformJson = objectJson.at("transform");

        object->getTransform().setPosition(jsonToVec3(transformJson.at("position")));
        object->getTransform().setRotation(jsonToVec3(transformJson.at("rotation")));
        object->getTransform().setScale(jsonToVec3(transformJson.at("scale")));

        saveData.addObject(std::move(mesh), std::move(object));
    }

    return saveData;
}