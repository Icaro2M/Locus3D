/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/io/SceneFragmentSerializer.h"

#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "kernel/common/Id.h"
#include "kernel/geometry/mesh/elements/Edge.h"
#include "kernel/geometry/mesh/elements/Face.h"
#include "kernel/geometry/mesh/elements/Loop.h"
#include "kernel/geometry/mesh/elements/Vertex.h"

#include <json.hpp>

#include <algorithm>
#include <cstdint>
#include <exception>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        constexpr const char* ClipboardMagic = "LOCUS3D_SCENE_FRAGMENT";

        using json = nlohmann::json;
        using kernel::geometry::EdgeHandle;
        using kernel::geometry::FaceHandle;
        using kernel::geometry::LoopHandle;
        using kernel::geometry::VertexHandle;

        [[nodiscard]] bool contains_node(
            const std::vector<SceneNodeId>& nodes,
            SceneNodeId node)
        {
            return std::find(nodes.begin(), nodes.end(), node) != nodes.end();
        }

        [[nodiscard]] std::vector<SceneNodeId> root_selection_nodes(
            const EditorScene& scene,
            const std::vector<SceneNodeId>& nodes)
        {
            std::vector<SceneNodeId> roots{};
            roots.reserve(nodes.size());

            for (const SceneNodeId node : nodes) {
                if (node.is_invalid()
                    || contains_node(roots, node)
                    || !scene.find_node(node)) {
                    continue;
                }

                bool shadowed = false;
                for (const SceneNodeId candidate : nodes) {
                    if (candidate != node
                        && scene.tree().is_ancestor(candidate, node)) {
                        shadowed = true;
                        break;
                    }
                }

                if (!shadowed) {
                    roots.push_back(node);
                }
            }

            return roots;
        }

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
                value[0].get<float>(),
                value[1].get<float>(),
                value[2].get<float>()
            };
            return true;
        }

        [[nodiscard]] bool json_to_quat(const json& value, glm::quat& out)
        {
            if (!value.is_array() || value.size() != 4u) {
                return false;
            }

            out = glm::quat{
                value[0].get<float>(),
                value[1].get<float>(),
                value[2].get<float>(),
                value[3].get<float>()
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

            if (!json_to_vec3(value.at("position"), position)
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
            if (!json_to_vec3(value.at("offset"), offset)) {
                return false;
            }

            out.offset = offset;
            out.custom = value.value("custom", false);
            return true;
        }

        [[nodiscard]] json mesh_to_json(const kernel::geometry::LEM& mesh)
        {
            json vertices = json::array();
            std::unordered_map<std::uint32_t, std::uint32_t> vertexMap{};

            for (std::uint32_t i = 0u;
                i < mesh.vertices().size();
                ++i) {
                const auto handle = VertexHandle{ i };
                if (!mesh.is_valid(handle)) {
                    continue;
                }

                const auto& vertex = mesh.vertices()[i];
                const std::uint32_t local =
                    static_cast<std::uint32_t>(vertices.size());
                vertexMap.emplace(i, local);
                vertices.push_back({
                    { "position", vec3_to_json(vertex.position) },
                    { "tag", vertex.tag },
                    { "selected", vertex.selected },
                    { "hidden", vertex.hidden }
                });
            }

            json edges = json::array();
            for (std::uint32_t i = 0u; i < mesh.edges().size(); ++i) {
                const auto handle = EdgeHandle{ i };
                if (!mesh.is_valid(handle)) {
                    continue;
                }

                const auto& edge = mesh.edges()[i];
                auto a = vertexMap.find(edge.vertexA.id.value);
                auto b = vertexMap.find(edge.vertexB.id.value);
                if (a == vertexMap.end() || b == vertexMap.end()) {
                    continue;
                }

                edges.push_back({
                    { "a", a->second },
                    { "b", b->second },
                    { "smooth", edge.smooth },
                    { "crease", edge.crease },
                    { "tag", edge.tag },
                    { "selected", edge.selected },
                    { "hidden", edge.hidden }
                });
            }

            json faces = json::array();
            for (std::uint32_t i = 0u; i < mesh.faces().size(); ++i) {
                const auto handle = FaceHandle{ i };
                if (!mesh.is_valid(handle)) {
                    continue;
                }

                json faceVertices = json::array();
                for (const LoopHandle loopHandle : mesh.face_loops(handle)) {
                    const auto& loop = mesh.loop(loopHandle);
                    auto vertex = vertexMap.find(loop.vertex.id.value);
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
                faces.push_back({
                    { "vertices", std::move(faceVertices) },
                    { "normal", vec3_to_json(face.normal) },
                    { "tag", face.tag },
                    { "selected", face.selected },
                    { "hidden", face.hidden }
                });
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
            if (!value.contains("vertices")
                || !value.contains("edges")
                || !value.contains("faces")) {
                return std::nullopt;
            }

            kernel::geometry::LEM mesh{};
            std::vector<VertexHandle> vertices{};

            for (const json& vertexJson : value.at("vertices")) {
                glm::vec3 position{};
                if (!json_to_vec3(vertexJson.at("position"), position)) {
                    return std::nullopt;
                }

                const VertexHandle handle = mesh.add_vertex(position);
                if (handle.is_invalid()) {
                    return std::nullopt;
                }

                auto& vertex = mesh.vertex(handle);
                vertex.tag = vertexJson.value("tag", 0u);
                vertex.selected = vertexJson.value("selected", false);
                vertex.hidden = vertexJson.value("hidden", false);
                vertices.push_back(handle);
            }

            for (const json& edgeJson : value.at("edges")) {
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
                edge.selected = edgeJson.value("selected", false);
                edge.hidden = edgeJson.value("hidden", false);
            }

            for (const json& faceJson : value.at("faces")) {
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
                glm::vec3 normal{};
                if (json_to_vec3(faceJson.at("normal"), normal)) {
                    face.normal = normal;
                }
                face.tag = faceJson.value("tag", 0u);
                face.selected = faceJson.value("selected", false);
                face.hidden = faceJson.value("hidden", false);
            }

            return mesh;
        }

        void capture_node_recursive(
            const EditorScene& scene,
            SceneNodeId nodeId,
            std::optional<SceneFragmentNodeId> parent,
            SceneFragment& fragment,
            SceneFragmentNodeId& nextFragmentId)
        {
            const SceneNode* node = scene.find_node(nodeId);
            if (!node) {
                return;
            }

            const SceneFragmentNodeId id = nextFragmentId++;

            SerializedNode serialized{};
            serialized.fragmentId = id;
            serialized.parentFragmentId = parent;
            serialized.type = node->type();
            serialized.transform = node->transform();
            serialized.pivot = node->pivot();
            serialized.metadata = node->metadata();

            if (const auto* meshNode =
                    dynamic_cast<const MeshNode*>(node)) {
                serialized.mesh = meshNode->mesh();
            }

            fragment.nodes.push_back(std::move(serialized));

            for (const SceneNodeId child : node->children()) {
                capture_node_recursive(
                    scene,
                    child,
                    id,
                    fragment,
                    nextFragmentId);
            }
        }

        [[nodiscard]] json node_to_json(const SerializedNode& node)
        {
            json result{
                { "id", node.fragmentId },
                { "type", node_type_name(node.type) },
                { "metadata", metadata_to_json(node.metadata) },
                { "transform", transform_to_json(node.transform) },
                { "pivot", pivot_to_json(node.pivot) }
            };

            if (node.parentFragmentId.has_value()) {
                result["parent"] = node.parentFragmentId.value();
            }

            if (node.mesh.has_value()) {
                result["mesh"] = mesh_to_json(node.mesh.value());
            }

            return result;
        }

    } // namespace

    SceneFragmentResult capture_scene_fragment(
        const EditorScene& scene,
        const std::vector<SceneNodeId>& roots)
    {
        const std::vector<SceneNodeId> selectedRoots =
            root_selection_nodes(scene, roots);

        if (selectedRoots.empty()) {
            return SceneFragmentResult::fail(
                "Cannot copy an empty scene node selection.");
        }

        SceneFragment fragment{};
        SceneFragmentNodeId nextFragmentId = 0u;

        for (const SceneNodeId root : selectedRoots) {
            capture_node_recursive(
                scene,
                root,
                std::nullopt,
                fragment,
                nextFragmentId);
        }

        std::string message{};
        if (!validate_scene_fragment(fragment, &message)) {
            return SceneFragmentResult::fail(std::move(message));
        }

        return SceneFragmentResult::ok(std::move(fragment));
    }

    bool validate_scene_fragment(
        const SceneFragment& fragment,
        std::string* message)
    {
        if (fragment.nodes.empty()) {
            if (message) {
                *message = "Scene fragment contains no nodes.";
            }
            return false;
        }

        std::unordered_set<SceneFragmentNodeId> ids{};
        ids.reserve(fragment.nodes.size());

        for (const SerializedNode& node : fragment.nodes) {
            if (node.fragmentId == InvalidSceneFragmentNodeId
                || !ids.insert(node.fragmentId).second) {
                if (message) {
                    *message = "Scene fragment contains invalid or duplicate node ids.";
                }
                return false;
            }

            if (node.type != NodeType::Empty
                && node.type != NodeType::Mesh) {
                if (message) {
                    *message = "Scene fragment contains an unsupported node type.";
                }
                return false;
            }

            if (node.type == NodeType::Mesh && !node.mesh.has_value()) {
                if (message) {
                    *message = "Scene fragment mesh node is missing mesh payload.";
                }
                return false;
            }
        }

        for (const SerializedNode& node : fragment.nodes) {
            if (node.parentFragmentId.has_value()
                && ids.find(node.parentFragmentId.value()) == ids.end()) {
                if (message) {
                    *message = "Scene fragment contains an invalid parent reference.";
                }
                return false;
            }

            if (node.parentFragmentId == node.fragmentId) {
                if (message) {
                    *message = "Scene fragment node cannot parent itself.";
                }
                return false;
            }
        }

        return true;
    }

    std::string serialize_scene_fragment(const SceneFragment& fragment)
    {
        json root{
            { "magic", ClipboardMagic },
            { "version", SceneFragmentVersion },
            { "nodes", json::array() }
        };

        for (const SerializedNode& node : fragment.nodes) {
            root["nodes"].push_back(node_to_json(node));
        }

        return root.dump();
    }

    SceneFragmentResult deserialize_scene_fragment(const std::string& text)
    {
        try {
            const json root = json::parse(text);

            if (root.value("magic", std::string{}) != ClipboardMagic) {
                return SceneFragmentResult::fail(
                    "Clipboard does not contain a Locus3D scene fragment.");
            }

            if (root.value("version", 0) != SceneFragmentVersion) {
                return SceneFragmentResult::fail(
                    "Clipboard scene fragment version is not supported.");
            }

            if (!root.contains("nodes") || !root.at("nodes").is_array()) {
                return SceneFragmentResult::fail(
                    "Clipboard scene fragment is missing nodes.");
            }

            SceneFragment fragment{};

            for (const json& nodeJson : root.at("nodes")) {
                SerializedNode node{};
                node.fragmentId =
                    nodeJson.at("id").get<SceneFragmentNodeId>();

                NodeType type = NodeType::Empty;
                if (!parse_node_type(
                        nodeJson.at("type").get<std::string>(),
                        type)) {
                    return SceneFragmentResult::fail(
                        "Clipboard scene fragment contains an unsupported node type.");
                }

                node.type = type;
                if (nodeJson.contains("parent")) {
                    node.parentFragmentId =
                        nodeJson.at("parent")
                            .get<SceneFragmentNodeId>();
                }

                node.metadata =
                    metadata_from_json(nodeJson.at("metadata"));

                if (!transform_from_json(
                        nodeJson.at("transform"),
                        node.transform)
                    || !pivot_from_json(
                        nodeJson.at("pivot"),
                        node.pivot)) {
                    return SceneFragmentResult::fail(
                        "Clipboard scene fragment contains invalid transform data.");
                }

                if (node.type == NodeType::Mesh) {
                    if (!nodeJson.contains("mesh")) {
                        return SceneFragmentResult::fail(
                            "Clipboard scene fragment mesh node is missing mesh data.");
                    }

                    std::optional<kernel::geometry::LEM> mesh =
                        mesh_from_json(nodeJson.at("mesh"));
                    if (!mesh.has_value()) {
                        return SceneFragmentResult::fail(
                            "Clipboard scene fragment contains invalid mesh data.");
                    }

                    node.mesh = std::move(mesh.value());
                }

                fragment.nodes.push_back(std::move(node));
            }

            std::string message{};
            if (!validate_scene_fragment(fragment, &message)) {
                return SceneFragmentResult::fail(std::move(message));
            }

            return SceneFragmentResult::ok(std::move(fragment));
        }
        catch (const std::exception& exception) {
            return SceneFragmentResult::fail(exception.what());
        }
    }

} // namespace locus::editor
