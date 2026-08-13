/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/ApplicationResult.h"
#include "application/document/DocumentSession.h"

#include <filesystem>

namespace locus::application {

    [[nodiscard]] ApplicationResult<void> save_document(
        DocumentSession& document);

    [[nodiscard]] ApplicationResult<void> save_document_as(
        DocumentSession& document);

    [[nodiscard]] ApplicationResult<void> open_document(
        DocumentSession& document);

    [[nodiscard]] ApplicationResult<void> open_document(
        DocumentSession& document,
        const std::filesystem::path& path);

} // namespace locus::application
