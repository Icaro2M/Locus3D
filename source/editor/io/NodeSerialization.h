/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/io/SerializedNode.h"

#include <json.hpp>

#include <optional>
#include <string>
#include <vector>

namespace locus::editor {

    /**
     * @brief Options that distinguish document persistence from clipboard.
     */
    struct NodeSerializationOptions {
        bool includeSelection = false;
        bool includeDerivedNormals = false;
    };

    [[nodiscard]] nlohmann::json serialized_node_to_json(
        const SerializedNode& node,
        NodeSerializationOptions options = {});

    [[nodiscard]] std::optional<SerializedNode> serialized_node_from_json(
        const nlohmann::json& value,
        std::string* message = nullptr);

    [[nodiscard]] bool validate_serialized_nodes(
        const std::vector<SerializedNode>& nodes,
        bool allowEmpty,
        std::string* message = nullptr);

} // namespace locus::editor
