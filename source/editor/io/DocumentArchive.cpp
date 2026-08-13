/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/io/DocumentArchive.h"

#include <utility>

namespace locus::editor {

    DocumentArchiveResult DocumentArchiveResult::ok(DocumentArchive archive)
    {
        return DocumentArchiveResult{ std::move(archive), {}, true };
    }

    DocumentArchiveResult DocumentArchiveResult::fail(std::string message)
    {
        return DocumentArchiveResult{ {}, std::move(message), false };
    }

} // namespace locus::editor
