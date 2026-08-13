/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/platform/FileDialog.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#endif

#include <algorithm>
#include <array>

namespace locus::application {

    namespace {

#ifdef _WIN32
        constexpr const wchar_t* LocusFilter =
            L"Locus3D Document (*.locus)\0*.locus\0All Files (*.*)\0*.*\0";

        [[nodiscard]] FileDialogResult dialog_error()
        {
            const DWORD error = CommDlgExtendedError();
            if (error == 0u) {
                return {};
            }

            return FileDialogResult{
                {},
                "Native file dialog failed.",
                false
            };
        }
#endif

    } // namespace

    FileDialogResult show_open_locus_dialog()
    {
#ifdef _WIN32
        std::array<wchar_t, MAX_PATH> buffer{};

        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = LocusFilter;
        dialog.lpstrFile = buffer.data();
        dialog.nMaxFile = static_cast<DWORD>(buffer.size());
        dialog.lpstrDefExt = L"locus";
        dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST |
            OFN_NOCHANGEDIR;

        if (GetOpenFileNameW(&dialog) == TRUE) {
            return FileDialogResult{
                std::filesystem::path{ buffer.data() },
                {},
                true
            };
        }

        return dialog_error();
#else
        return FileDialogResult{
            {},
            "Native file dialogs are not implemented on this platform.",
            false
        };
#endif
    }

    FileDialogResult show_save_locus_dialog(
        const std::filesystem::path& suggestedPath)
    {
#ifdef _WIN32
        std::array<wchar_t, MAX_PATH> buffer{};
        const std::wstring suggested = suggestedPath.wstring();
        const std::size_t count =
            std::min(buffer.size() - 1u, suggested.size());
        std::copy_n(suggested.c_str(), count, buffer.data());

        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.lpstrFilter = LocusFilter;
        dialog.lpstrFile = buffer.data();
        dialog.nMaxFile = static_cast<DWORD>(buffer.size());
        dialog.lpstrDefExt = L"locus";
        dialog.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT |
            OFN_NOCHANGEDIR;

        if (GetSaveFileNameW(&dialog) == TRUE) {
            return FileDialogResult{
                std::filesystem::path{ buffer.data() },
                {},
                true
            };
        }

        return dialog_error();
#else
        (void)suggestedPath;
        return FileDialogResult{
            {},
            "Native file dialogs are not implemented on this platform.",
            false
        };
#endif
    }

} // namespace locus::application
