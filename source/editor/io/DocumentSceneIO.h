/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/Editor.h"
#include "editor/io/DocumentArchive.h"

#include <string>

namespace locus::editor {

    [[nodiscard]] DocumentArchiveResult capture_document_archive(
        const EditorScene& scene);

    [[nodiscard]] bool validate_document_archive(
        const DocumentArchive& archive,
        std::string* message = nullptr);

    [[nodiscard]] DocumentArchiveResult apply_document_archive(
        Editor& editor,
        const DocumentArchive& archive);

} // namespace locus::editor
