/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/io/SerializedNode.h"

#include <string>
#include <vector>

namespace locus::editor {

    constexpr int Locus3DDocumentVersion = 1;

    /**
     * @brief Complete persistent editor document payload.
     */
    struct DocumentArchive {
        int version = Locus3DDocumentVersion;
        std::vector<SerializedNode> nodes{};
    };

    struct DocumentArchiveResult {
        DocumentArchive archive{};
        std::string message{};
        bool success = false;

        [[nodiscard]] static DocumentArchiveResult ok(
            DocumentArchive archive);
        [[nodiscard]] static DocumentArchiveResult fail(
            std::string message);
    };

} // namespace locus::editor
