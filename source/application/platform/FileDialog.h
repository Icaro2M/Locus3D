/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <string>

namespace locus::application {

    struct FileDialogResult {
        std::filesystem::path path{};
        std::string message{};
        bool accepted = false;
    };

    [[nodiscard]] FileDialogResult show_open_locus_dialog();
    [[nodiscard]] FileDialogResult show_save_locus_dialog(
        const std::filesystem::path& suggestedPath = {});

} // namespace locus::application
