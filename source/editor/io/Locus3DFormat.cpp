/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/io/Locus3DFormat.h"

#include "editor/io/DocumentSceneIO.h"
#include "editor/io/NodeSerialization.h"

#include <json.hpp>

#include <exception>
#include <string>
#include <utility>

namespace locus::editor {

    namespace {

        using json = nlohmann::json;

    } // namespace

    std::string serialize_locus_document(const DocumentArchive& archive)
    {
        json document{
            { "nodes", json::array() }
        };

        for (const SerializedNode& node : archive.nodes) {
            document["nodes"].push_back(
                serialized_node_to_json(
                    node,
                    NodeSerializationOptions{
                        false,
                        false }));
        }

        json root{
            { "format", Locus3DDocumentMagic },
            { "version", Locus3DDocumentVersion },
            { "document", std::move(document) }
        };

        return root.dump(2);
    }

    DocumentArchiveResult deserialize_locus_document(const std::string& text)
    {
        try {
            const json root = json::parse(text);

            if (!root.is_object()
                || root.value("format", std::string{})
                != Locus3DDocumentMagic) {
                return DocumentArchiveResult::fail(
                    "File is not a Locus3D document.");
            }

            const int version = root.value("version", 0);
            if (version != Locus3DDocumentVersion) {
                return DocumentArchiveResult::fail(
                    "Locus3D document version is not supported.");
            }

            if (!root.contains("document")
                || !root.at("document").is_object()
                || !root.at("document").contains("nodes")
                || !root.at("document").at("nodes").is_array()) {
                return DocumentArchiveResult::fail(
                    "Locus3D document is missing node data.");
            }

            DocumentArchive archive{};
            archive.version = version;

            for (const json& nodeJson :
                root.at("document").at("nodes")) {
                std::string message{};
                std::optional<SerializedNode> node =
                    serialized_node_from_json(nodeJson, &message);
                if (!node.has_value()) {
                    if (message.empty()) {
                        message = "Locus3D document contains invalid node data.";
                    }
                    return DocumentArchiveResult::fail(std::move(message));
                }

                archive.nodes.push_back(std::move(node.value()));
            }

            std::string message{};
            if (!validate_document_archive(archive, &message)) {
                return DocumentArchiveResult::fail(std::move(message));
            }

            return DocumentArchiveResult::ok(std::move(archive));
        }
        catch (const std::exception& exception) {
            return DocumentArchiveResult::fail(exception.what());
        }
    }

} // namespace locus::editor
