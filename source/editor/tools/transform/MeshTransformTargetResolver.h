/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/tools/transform/MeshTransformTarget.h"

#include <optional>
#include <string>

namespace locus::editor {

    class EditorScene;
    class SelectionState;

    struct MeshTransformTargetResolveResult {
        bool success = false;
        MeshTransformTarget target{};
        std::string message{};

        [[nodiscard]] static MeshTransformTargetResolveResult ok(
            MeshTransformTarget resolved);

        [[nodiscard]] static MeshTransformTargetResolveResult fail(
            std::string message);
    };

    /**
     * @brief Resolves selected mesh components into unique editable vertices.
     */
    class MeshTransformTargetResolver {
    public:
        [[nodiscard]] static MeshTransformTargetResolveResult resolve(
            const EditorScene& scene,
            const SelectionState& selection);
    };

} // namespace locus::editor
