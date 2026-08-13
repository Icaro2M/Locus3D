/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/scene/NodeMetadata.h"
#include "editor/scene/NodePivot.h"
#include "editor/scene/NodeTransform.h"
#include "editor/scene/NodeType.h"
#include "kernel/geometry/mesh/LEM.h"

#include <cstdint>
#include <optional>

namespace locus::editor {

    using SerializedNodeId = std::uint64_t;

    constexpr SerializedNodeId InvalidSerializedNodeId = UINT64_MAX;

    /**
     * @brief Persistent local-id representation of an editor scene node.
     *
     * The id fields are local to a fragment/archive and never store runtime
     * SceneNodeId values.
     */
    struct SerializedNode {
        SerializedNodeId id = InvalidSerializedNodeId;
        std::optional<SerializedNodeId> parentId{};
        NodeType type = NodeType::Empty;
        NodeTransform transform{};
        NodePivot pivot{};
        NodeMetadata metadata{};
        std::optional<kernel::geometry::LEM> mesh{};
    };

} // namespace locus::editor
