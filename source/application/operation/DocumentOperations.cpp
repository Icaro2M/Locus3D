/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/operation/DocumentOperations.h"

#include "application/document/DocumentIO.h"
#include "application/platform/FileDialog.h"
#include "editor/io/DocumentSceneIO.h"

#include <iostream>
#include <utility>

namespace locus::application {

    namespace {

        [[nodiscard]] ApplicationError runtime_error(std::string message)
        {
            return ApplicationError::make(
                ApplicationErrorCode::RuntimeFailure,
                std::move(message));
        }

        [[nodiscard]] ApplicationResult<void> save_document_to_path(
            DocumentSession& document,
            const std::filesystem::path& path)
        {
            editor::DocumentArchiveResult archive =
                editor::capture_document_archive(document.editor().scene());
            if (!archive.success) {
                return runtime_error(archive.message);
            }

            const std::filesystem::path target =
                ensure_locus_extension(path);
            const ApplicationResult<void> writeResult =
                write_locus_document_atomic(target, archive.archive);
            if (!writeResult) {
                return writeResult.error();
            }

            document.mark_saved(target);

            std::cout
                << "[document] saved path=\""
                << document.path().string()
                << "\" dirty="
                << document.is_dirty()
                << '\n';

            return {};
        }

    } // namespace

    ApplicationResult<void> save_document(DocumentSession& document)
    {
        if (!document.has_path()) {
            return save_document_as(document);
        }

        return save_document_to_path(document, document.path());
    }

    ApplicationResult<void> save_document_as(DocumentSession& document)
    {
        const FileDialogResult dialog =
            show_save_locus_dialog(document.path());
        if (!dialog.accepted) {
            if (!dialog.message.empty()) {
                return runtime_error(dialog.message);
            }
            std::cout << "[document] save cancelled\n";
            return {};
        }

        return save_document_to_path(document, dialog.path);
    }

    ApplicationResult<void> open_document(DocumentSession& document)
    {
        const FileDialogResult dialog = show_open_locus_dialog();
        if (!dialog.accepted) {
            if (!dialog.message.empty()) {
                return runtime_error(dialog.message);
            }
            std::cout << "[document] open cancelled\n";
            return {};
        }

        return open_document(document, dialog.path);
    }

    ApplicationResult<void> open_document(
        DocumentSession& document,
        const std::filesystem::path& path)
    {
        const ApplicationResult<editor::DocumentArchive> archive =
            read_locus_document(path);
        if (!archive) {
            return archive.error();
        }

        editor::DocumentArchiveResult loadResult =
            editor::apply_document_archive(
                document.editor(),
                archive.value());
        if (!loadResult.success) {
            return runtime_error(loadResult.message);
        }

        document.mark_loaded(path);
        document.editor().mark_dirty(editor::EditorDirtyFlags::All);

        std::cout
            << "[document] opened path=\""
            << document.path().string()
            << "\" dirty="
            << document.is_dirty()
            << '\n';

        return {};
    }

} // namespace locus::application
