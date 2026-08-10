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
#include <string>
#include <vector>

namespace locus::editor {

    using SceneFragmentNodeId = std::uint64_t;

    constexpr SceneFragmentNodeId InvalidSceneFragmentNodeId =
        UINT64_MAX;

    struct SerializedNode {
        SceneFragmentNodeId fragmentId = InvalidSceneFragmentNodeId;
        std::optional<SceneFragmentNodeId> parentFragmentId{};
        NodeType type = NodeType::Empty;
        NodeTransform transform{};
        NodePivot pivot{};
        NodeMetadata metadata{};
        std::optional<kernel::geometry::LEM> mesh{};
    };

    struct SceneFragment {
        std::vector<SerializedNode> nodes{};
    };

    struct SceneFragmentResult {
        SceneFragment fragment{};
        std::string message{};
        bool success = false;

        [[nodiscard]] static SceneFragmentResult ok(SceneFragment fragment);
        [[nodiscard]] static SceneFragmentResult fail(std::string message);
    };

} // namespace locus::editor
