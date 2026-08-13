/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/io/NodeSerialization.h"

#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Face.h"
#include "kernel/geometry/mesh/elements/Loop.h"
#include "kernel/geometry/mesh/elements/Vertex.h"
#include "kernel/geometry/render/NormalBuilder.h"

#include <glm/geometric.hpp>

#include <exception>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        using json = nlohmann::json;
        using kernel::geometry::EdgeHandle;
        using kernel::geometry::FaceHandle;
        using kernel::geometry::LoopHandle;
        using kernel::geometry::VertexHandle;

        [[nodiscard]] json vec3_to_json(const glm::vec3& value)
        {
            return json::array({ value.x, value.y, value.z });
        }

        [[nodiscard]] json quat_to_json(const glm::quat& value)
        {
            return json::array({ value.w, value.x, value.y, value.z });
        }

        [[nodiscard]] bool json_to_vec3(const json& value, glm::vec3& out)
        {
            if (!value.is_array() || value.size() != 3u) {
                return false;
            }

            out = glm::vec3{
                value.at(0).get<float>(),
                value.at(1).get<float>(),
                value.at(2).get<float>()
            };
            return true;
        }

        [[nodiscard]] bool json_to_quat(const json& value, glm::quat& out)
        {
            if (!value.is_array() || value.size() != 4u) {
                return false;
            }

            out = glm::quat{
                value.at(0).get<float>(),
                value.at(1).get<float>(),
                value.at(2).get<float>(),
                value.at(3).get<float>()
            };
            return true;
        }

        [[nodiscard]] const char* node_type_name(NodeType type) noexcept
        {
            switch (type) {
            case NodeType::Empty:
                return "empty";
            case NodeType::Mesh:
                return "mesh";
            }

            return "unknown";
        }

        [[nodiscard]] bool parse_node_type(
            const std::string& value,
            NodeType& out) noexcept
        {
            if (value == "empty") {
                out = NodeType::Empty;
                return true;
            }
            if (value == "mesh") {
                out = NodeType::Mesh;
                return true;
            }
            return false;
        }

        [[nodiscard]] json metadata_to_json(const NodeMetadata& metadata)
        {
            return json{
                { "name", metadata.name },
                { "visible", metadata.visible },
                { "locked", metadata.locked },
                { "selectable", metadata.selectable },
                { "expanded", metadata.expanded }
            };
        }

        [[nodiscard]] NodeMetadata metadata_from_json(const json& value)
        {
            NodeMetadata metadata{};
            metadata.name = value.value("name", std::string{});
            metadata.visible = value.value("visible", true);
            metadata.locked = value.value("locked", false);
            metadata.selectable = value.value("selectable", true);
            metadata.expanded = value.value("expanded", true);
            return metadata;
        }

        [[nodiscard]] json transform_to_json(const NodeTransform& transform)
        {
            return json{
                { "position", vec3_to_json(transform.position()) },
                { "rotation", quat_to_json(transform.rotation()) },
                { "scale", vec3_to_json(transform.scale()) }
            };
        }

        [[nodiscard]] bool transform_from_json(
            const json& value,
            NodeTransform& out)
        {
            glm::vec3 position{};
            glm::quat rotation{};
            glm::vec3 scale{};

            if (!value.is_object()
                || !json_to_vec3(value.at("position"), position)
                || !json_to_quat(value.at("rotation"), rotation)
                || !json_to_vec3(value.at("scale"), scale)) {
                return false;
            }

            out.set_position(position);
            out.set_rotation(rotation);
            out.set_scale(scale);
            return true;
        }

        [[nodiscard]] json pivot_to_json(const NodePivot& pivot)
        {
            return json{
                { "offset", vec3_to_json(pivot.offset) },
                { "custom", pivot.custom }
            };
        }

        [[nodiscard]] bool pivot_from_json(const json& value, NodePivot& out)
        {
            glm::vec3 offset{};
            if (!value.is_object() || !json_to_vec3(value.at("offset"), offset)) {
                return false;
            }

            out.offset = offset;
            out.custom = value.value("custom", false);
            return true;
        }

        [[nodiscard]] json mesh_to_json(
            const kernel::geometry::LEM& mesh,
            NodeSerializationOptions options)
        {
            json vertices = json::array();
            std::unordered_map<std::uint32_t, std::uint32_t> vertexMap{};

            for (std::uint32_t i = 0u; i < mesh.vertices().size(); ++i) {
                const VertexHandle handle{ i };
                if (!mesh.is_valid(handle)) {
                    continue;
                }

                const auto& vertex = mesh.vertices()[i];
                const std::uint32_t local =
                    static_cast<std::uint32_t>(vertices.size());
                vertexMap.emplace(i, local);

                json vertexJson{
                    { "position", vec3_to_json(vertex.position) },
                    { "tag", vertex.tag },
                    { "hidden", vertex.hidden }
                };
                if (options.includeSelection) {
                    vertexJson["selected"] = vertex.selected;
                }
                vertices.push_back(std::move(vertexJson));
            }

            json edges = json::array();
            for (std::uint32_t i = 0u; i < mesh.edges().size(); ++i) {
                const EdgeHandle handle{ i };
                if (!mesh.is_valid(handle)) {
                    continue;
                }

                const auto& edge = mesh.edges()[i];
                const auto a = vertexMap.find(edge.vertexA.id.value);
                const auto b = vertexMap.find(edge.vertexB.id.value);
                if (a == vertexMap.end() || b == vertexMap.end()) {
                    continue;
                }

                json edgeJson{
                    { "a", a->second },
                    { "b", b->second },
                    { "smooth", edge.smooth },
                    { "crease", edge.crease },
                    { "tag", edge.tag },
                    { "hidden", edge.hidden }
                };
                if (options.includeSelection) {
                    edgeJson["selected"] = edge.selected;
                }
                edges.push_back(std::move(edgeJson));
            }

            json faces = json::array();
            for (std::uint32_t i = 0u; i < mesh.faces().size(); ++i) {
                const FaceHandle handle{ i };
                if (!mesh.is_valid(handle)) {
                    continue;
                }

                json faceVertices = json::array();
                for (const LoopHandle loopHandle : mesh.face_loops(handle)) {
                    const auto& loop = mesh.loop(loopHandle);
                    const auto vertex = vertexMap.find(loop.vertex.id.value);
                    if (vertex == vertexMap.end()) {
                        faceVertices.clear();
                        break;
                    }
                    faceVertices.push_back(vertex->second);
                }

                if (faceVertices.size() < 3u) {
                    continue;
                }

                const auto& face = mesh.faces()[i];
                json faceJson{
                    { "vertices", std::move(faceVertices) },
                    { "tag", face.tag },
                    { "hidden", face.hidden }
                };
                if (options.includeDerivedNormals) {
                    faceJson["normal"] = vec3_to_json(face.normal);
                }
                if (options.includeSelection) {
                    faceJson["selected"] = face.selected;
                }
                faces.push_back(std::move(faceJson));
            }

            return json{
                { "vertices", std::move(vertices) },
                { "edges", std::move(edges) },
                { "faces", std::move(faces) }
            };
        }

        [[nodiscard]] std::optional<kernel::geometry::LEM>
        mesh_from_json(const json& value)
        {
            if (!value.is_object()
                || !value.contains("vertices")
                || !value.contains("edges")
                || !value.contains("faces")
                || !value.at("vertices").is_array()
                || !value.at("edges").is_array()
                || !value.at("faces").is_array()) {
                return std::nullopt;
            }

            kernel::geometry::LEM mesh{};
            std::vector<VertexHandle> vertices{};

            for (const json& vertexJson : value.at("vertices")) {
                glm::vec3 position{};
                if (!vertexJson.is_object()
                    || !json_to_vec3(vertexJson.at("position"), position)) {
                    return std::nullopt;
                }

                const VertexHandle handle = mesh.add_vertex(position);
                if (handle.is_invalid()) {
                    return std::nullopt;
                }

                auto& vertex = mesh.vertex(handle);
                vertex.tag = vertexJson.value("tag", 0u);
                vertex.hidden = vertexJson.value("hidden", false);
                vertex.selected = vertexJson.value("selected", false);
                vertices.push_back(handle);
            }

            for (const json& edgeJson : value.at("edges")) {
                if (!edgeJson.is_object()) {
                    return std::nullopt;
                }

                const auto a = edgeJson.at("a").get<std::size_t>();
                const auto b = edgeJson.at("b").get<std::size_t>();
                if (a >= vertices.size() || b >= vertices.size()) {
                    return std::nullopt;
                }

                const EdgeHandle handle =
                    mesh.find_or_create_edge(vertices[a], vertices[b]);
                if (handle.is_invalid()) {
                    return std::nullopt;
                }

                auto& edge = mesh.edge(handle);
                edge.smooth = edgeJson.value("smooth", false);
                edge.crease = edgeJson.value("crease", 0.0f);
                edge.tag = edgeJson.value("tag", 0u);
                edge.hidden = edgeJson.value("hidden", false);
                edge.selected = edgeJson.value("selected", false);
            }

            for (const json& faceJson : value.at("faces")) {
                if (!faceJson.is_object()
                    || !faceJson.contains("vertices")
                    || !faceJson.at("vertices").is_array()) {
                    return std::nullopt;
                }

                std::vector<VertexHandle> faceVertices{};
                for (const json& vertexJson : faceJson.at("vertices")) {
                    const auto index = vertexJson.get<std::size_t>();
                    if (index >= vertices.size()) {
                        return std::nullopt;
                    }
                    faceVertices.push_back(vertices[index]);
                }

                const FaceHandle handle = mesh.add_face(faceVertices);
                if (handle.is_invalid()) {
                    return std::nullopt;
                }

                auto& face = mesh.face(handle);
                face.tag = faceJson.value("tag", 0u);
                face.hidden = faceJson.value("hidden", false);
                face.selected = faceJson.value("selected", false);
            }

            kernel::geometry::NormalBuilder::rebuild_face_normals(mesh);
            return mesh;
        }

    } // namespace

    nlohmann::json serialized_node_to_json(
        const SerializedNode& node,
        NodeSerializationOptions options)
    {
        json result{
            { "id", node.id },
            { "type", node_type_name(node.type) },
            { "metadata", metadata_to_json(node.metadata) },
            { "transform", transform_to_json(node.transform) },
            { "pivot", pivot_to_json(node.pivot) }
        };

        if (node.parentId.has_value()) {
            result["parent"] = node.parentId.value();
        }

        if (node.mesh.has_value()) {
            result["mesh"] = mesh_to_json(node.mesh.value(), options);
        }

        return result;
    }

    std::optional<SerializedNode> serialized_node_from_json(
        const nlohmann::json& value,
        std::string* message)
    {
        try {
            if (!value.is_object()) {
                if (message) {
                    *message = "Serialized node is not an object.";
                }
                return std::nullopt;
            }

            SerializedNode node{};
            node.id = value.at("id").get<SerializedNodeId>();

            NodeType type = NodeType::Empty;
            if (!parse_node_type(value.at("type").get<std::string>(), type)) {
                if (message) {
                    *message = "Serialized node contains an unsupported node type.";
                }
                return std::nullopt;
            }
            node.type = type;

            if (value.contains("parent")) {
                node.parentId = value.at("parent").get<SerializedNodeId>();
            }

            node.metadata = metadata_from_json(value.at("metadata"));

            if (!transform_from_json(value.at("transform"), node.transform)
                || !pivot_from_json(value.at("pivot"), node.pivot)) {
                if (message) {
                    *message = "Serialized node contains invalid transform data.";
                }
                return std::nullopt;
            }

            if (node.type == NodeType::Mesh) {
                if (!value.contains("mesh")) {
                    if (message) {
                        *message = "Serialized mesh node is missing mesh data.";
                    }
                    return std::nullopt;
                }

                std::optional<kernel::geometry::LEM> mesh =
                    mesh_from_json(value.at("mesh"));
                if (!mesh.has_value()) {
                    if (message) {
                        *message = "Serialized node contains invalid mesh data.";
                    }
                    return std::nullopt;
                }

                node.mesh = std::move(mesh.value());
            }

            return node;
        }
        catch (const std::exception& exception) {
            if (message) {
                *message = exception.what();
            }
            return std::nullopt;
        }
    }

    bool validate_serialized_nodes(
        const std::vector<SerializedNode>& nodes,
        bool allowEmpty,
        std::string* message)
    {
        if (nodes.empty() && !allowEmpty) {
            if (message) {
                *message = "Serialized node list is empty.";
            }
            return false;
        }

        std::unordered_set<SerializedNodeId> ids{};
        ids.reserve(nodes.size());

        for (const SerializedNode& node : nodes) {
            if (node.id == InvalidSerializedNodeId
                || !ids.insert(node.id).second) {
                if (message) {
                    *message = "Serialized node list contains invalid or duplicate node ids.";
                }
                return false;
            }

            if (node.type != NodeType::Empty && node.type != NodeType::Mesh) {
                if (message) {
                    *message = "Serialized node list contains an unsupported node type.";
                }
                return false;
            }

            if (node.type == NodeType::Mesh && !node.mesh.has_value()) {
                if (message) {
                    *message = "Serialized mesh node is missing mesh payload.";
                }
                return false;
            }
        }

        for (const SerializedNode& node : nodes) {
            if (node.parentId.has_value()
                && ids.find(node.parentId.value()) == ids.end()) {
                if (message) {
                    *message = "Serialized node list contains an invalid parent reference.";
                }
                return false;
            }

            if (node.parentId == node.id) {
                if (message) {
                    *message = "Serialized node cannot parent itself.";
                }
                return false;
            }
        }

        return true;
    }

} // namespace locus::editor
