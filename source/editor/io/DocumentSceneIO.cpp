/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/io/DocumentSceneIO.h"

#include "editor/io/NodeSerialization.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>

namespace locus::editor {

    namespace {

        void capture_node_recursive(
            const EditorScene& scene,
            SceneNodeId nodeId,
            std::optional<SerializedNodeId> parent,
            DocumentArchive& archive,
            SerializedNodeId& nextId)
        {
            const SceneNode* node = scene.find_node(nodeId);
            if (!node) {
                return;
            }

            const SerializedNodeId id = nextId++;

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

            archive.nodes.push_back(std::move(serialized));

            for (const SceneNodeId child : node->children()) {
                capture_node_recursive(
                    scene,
                    child,
                    id,
                    archive,
                    nextId);
            }
        }

        [[nodiscard]] bool build_scene_from_archive(
            const DocumentArchive& archive,
            EditorScene& scene,
            std::string* message)
        {
            std::unordered_map<SerializedNodeId, SceneNodeId> ids{};
            ids.reserve(archive.nodes.size());

            for (const SerializedNode& serialized : archive.nodes) {
                SceneNodeId sceneId{};
                if (serialized.type == NodeType::Mesh) {
                    sceneId = scene.create_mesh(serialized.metadata.name);
                    MeshNode* node = scene.find_mesh(sceneId);
                    if (!node || !serialized.mesh.has_value()) {
                        if (message) {
                            *message = "Failed to create archived mesh node.";
                        }
                        return false;
                    }

                    node->mesh() = serialized.mesh.value();
                    node->bump_mesh_revision();
                }
                else {
                    sceneId = scene.create_empty(serialized.metadata.name);
                }

                SceneNode* node = scene.find_node(sceneId);
                if (!node) {
                    if (message) {
                        *message = "Failed to create archived scene node.";
                    }
                    return false;
                }

                node->metadata() = serialized.metadata;
                node->transform() = serialized.transform;
                node->pivot() = serialized.pivot;
                node->mark_dirty(EditorDirtyFlags::All);
                ids.emplace(serialized.id, sceneId);
            }

            for (const SerializedNode& serialized : archive.nodes) {
                if (!serialized.parentId.has_value()) {
                    continue;
                }

                const auto child = ids.find(serialized.id);
                const auto parent = ids.find(serialized.parentId.value());
                if (child == ids.end() || parent == ids.end()
                    || !scene.reparent(child->second, parent->second)) {
                    if (message) {
                        *message = "Failed to recreate archived hierarchy.";
                    }
                    return false;
                }
            }

            return true;
        }

    } // namespace

    DocumentArchiveResult capture_document_archive(const EditorScene& scene)
    {
        DocumentArchive archive{};
        SerializedNodeId nextId = 0u;

        for (const SceneNodeId root : scene.tree().roots()) {
            capture_node_recursive(
                scene,
                root,
                std::nullopt,
                archive,
                nextId);
        }

        std::string message{};
        if (!validate_document_archive(archive, &message)) {
            return DocumentArchiveResult::fail(std::move(message));
        }

        return DocumentArchiveResult::ok(std::move(archive));
    }

    bool validate_document_archive(
        const DocumentArchive& archive,
        std::string* message)
    {
        if (archive.version != Locus3DDocumentVersion) {
            if (message) {
                *message = "Locus3D document version is not supported.";
            }
            return false;
        }

        return validate_serialized_nodes(
            archive.nodes,
            true,
            message);
    }

    DocumentArchiveResult apply_document_archive(
        Editor& editor,
        const DocumentArchive& archive)
    {
        std::string message{};
        if (!validate_document_archive(archive, &message)) {
            return DocumentArchiveResult::fail(std::move(message));
        }

        EditorScene scene{};
        if (!build_scene_from_archive(archive, scene, &message)) {
            return DocumentArchiveResult::fail(std::move(message));
        }

        editor.replace_scene(std::move(scene));
        editor.selection().clear();
        editor.clear_dirty();
        return DocumentArchiveResult::ok(archive);
    }

} // namespace locus::editor
