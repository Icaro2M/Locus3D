/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/ApplicationResult.h"
#include "editor/io/DocumentArchive.h"

#include <filesystem>

namespace locus::application {

    [[nodiscard]] std::filesystem::path ensure_locus_extension(
        std::filesystem::path path);

    [[nodiscard]] ApplicationResult<void> write_locus_document_atomic(
        const std::filesystem::path& path,
        const editor::DocumentArchive& archive);

    [[nodiscard]] ApplicationResult<editor::DocumentArchive>
        read_locus_document(const std::filesystem::path& path);

} // namespace locus::application
