/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/io/SceneFragment.h"

#include <utility>

namespace locus::editor {

    SceneFragmentResult SceneFragmentResult::ok(SceneFragment fragment)
    {
        SceneFragmentResult result{};
        result.fragment = std::move(fragment);
        result.success = true;
        return result;
    }

    SceneFragmentResult SceneFragmentResult::fail(std::string message)
    {
        SceneFragmentResult result{};
        result.message = std::move(message);
        result.success = false;
        return result;
    }

} // namespace locus::editor
