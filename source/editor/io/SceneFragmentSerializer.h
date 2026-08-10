/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/io/SceneFragment.h"
#include "editor/scene/EditorScene.h"
#include "editor/scene/SceneNodeId.h"

#include <string>
#include <vector>

namespace locus::editor {

    constexpr int SceneFragmentVersion = 1;

    [[nodiscard]] SceneFragmentResult capture_scene_fragment(
        const EditorScene& scene,
        const std::vector<SceneNodeId>& roots);

    [[nodiscard]] bool validate_scene_fragment(
        const SceneFragment& fragment,
        std::string* message = nullptr);

    [[nodiscard]] std::string serialize_scene_fragment(
        const SceneFragment& fragment);

    [[nodiscard]] SceneFragmentResult deserialize_scene_fragment(
        const std::string& text);

} // namespace locus::editor
