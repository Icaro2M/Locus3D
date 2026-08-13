/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/io/SceneFragmentSerializer.h"

#include "editor/io/NodeSerialization.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"

#include <json.hpp>

#include <algorithm>
#include <exception>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace locus::editor {

    namespace {

        constexpr const char* ClipboardMagic = "LOCUS3D_SCENE_FRAGMENT";

        using json = nlohmann::json;

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
            serialized.id = id;
            serialized.parentId = parent;
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

        return validate_serialized_nodes(
            fragment.nodes,
            false,
            message);
    }

    std::string serialize_scene_fragment(const SceneFragment& fragment)
    {
        json root{
            { "magic", ClipboardMagic },
            { "version", SceneFragmentVersion },
            { "nodes", json::array() }
        };

        for (const SerializedNode& node : fragment.nodes) {
            root["nodes"].push_back(
                serialized_node_to_json(
                    node,
                    NodeSerializationOptions{
                        true,
                        true }));
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
                std::string message{};
                std::optional<SerializedNode> node =
                    serialized_node_from_json(nodeJson, &message);
                if (!node.has_value()) {
                    if (message.empty()) {
                        message = "Clipboard scene fragment contains invalid node data.";
                    }
                    return SceneFragmentResult::fail(std::move(message));
                }

                fragment.nodes.push_back(std::move(node.value()));
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
