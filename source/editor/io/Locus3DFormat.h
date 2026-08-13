/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "editor/io/DocumentArchive.h"

#include <string>

namespace locus::editor {

    constexpr const char* Locus3DDocumentMagic = "Locus3D";

    [[nodiscard]] std::string serialize_locus_document(
        const DocumentArchive& archive);

    [[nodiscard]] DocumentArchiveResult deserialize_locus_document(
        const std::string& text);

} // namespace locus::editor
