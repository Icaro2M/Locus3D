/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/io/SerializedNode.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace locus::editor {

    using SceneFragmentNodeId = SerializedNodeId;

    constexpr SceneFragmentNodeId InvalidSceneFragmentNodeId =
        InvalidSerializedNodeId;

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
