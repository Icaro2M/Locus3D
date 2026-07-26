/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/document/DocumentId.h"
#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/history/HistoryStack.h"
#include "editor/sync/EditorSync.h"
#include "editor/tools/management/ToolManager.h"
#include "editor/tools/management/ToolRegistry.h"

#include <filesystem>
#include <string>

namespace locus::application {

    /**
     * @brief Owns all editor and persistence state for one open document.
     *
     * Document dirty state represents unsaved persistent changes and remains
     * independent from editor dirty flags used for render synchronization.
     */
    class DocumentSession {
    public:
        /**
         * @brief Creates a headless document session.
         *
         * @param id Stable application document identifier.
         */
        explicit DocumentSession(DocumentId id);

        DocumentSession(const DocumentSession&) = delete;
        DocumentSession& operator=(const DocumentSession&) = delete;
        DocumentSession(DocumentSession&&) = delete;
        DocumentSession& operator=(DocumentSession&&) = delete;

        /**
         * @brief Returns this session's stable identifier.
         *
         * @return Document identifier.
         */
        [[nodiscard]] DocumentId id() const noexcept;

        /**
         * @brief Returns the editor owned by this document.
         *
         * @return Mutable editor reference.
         */
        [[nodiscard]] editor::Editor& editor() noexcept;

        /**
         * @brief Returns the editor owned by this document.
         *
         * @return Read-only editor reference.
         */
        [[nodiscard]] const editor::Editor& editor() const noexcept;

        /**
         * @brief Returns the document command dispatcher.
         *
         * @return Mutable dispatcher reference.
         */
        [[nodiscard]] editor::CommandDispatcher&
            command_dispatcher() noexcept;

        /**
         * @brief Returns the document command dispatcher.
         *
         * @return Read-only dispatcher reference.
         */
        [[nodiscard]] const editor::CommandDispatcher&
            command_dispatcher() const noexcept;

        /**
         * @brief Returns the document undo and redo history.
         *
         * @return Mutable history reference.
         */
        [[nodiscard]] editor::HistoryStack& history() noexcept;

        /**
         * @brief Returns the document undo and redo history.
         *
         * @return Read-only history reference.
         */
        [[nodiscard]] const editor::HistoryStack& history() const noexcept;

        /**
         * @brief Returns the registry that owns document tool factories.
         *
         * @return Mutable registry reference.
         */
        [[nodiscard]] editor::ToolRegistry& tool_registry() noexcept;

        /**
         * @brief Returns the registry that owns document tool factories.
         *
         * @return Read-only registry reference.
         */
        [[nodiscard]] const editor::ToolRegistry&
            tool_registry() const noexcept;

        /**
         * @brief Returns the document tool manager.
         *
         * @return Mutable tool manager reference.
         */
        [[nodiscard]] editor::ToolManager& tool_manager() noexcept;

        /**
         * @brief Returns the document tool manager.
         *
         * @return Read-only tool manager reference.
         */
        [[nodiscard]] const editor::ToolManager&
            tool_manager() const noexcept;

        /**
         * @brief Returns editor-to-graphics synchronization state.
         *
         * @return Mutable synchronization facade reference.
         */
        [[nodiscard]] editor::EditorSync& editor_sync() noexcept;

        /**
         * @brief Returns editor-to-graphics synchronization state.
         *
         * @return Read-only synchronization facade reference.
         */
        [[nodiscard]] const editor::EditorSync&
            editor_sync() const noexcept;

        /**
         * @brief Checks whether persistent changes remain unsaved.
         *
         * @return True when the document must be saved.
         */
        [[nodiscard]] bool is_dirty() const noexcept;

        /**
         * @brief Checks whether the document has an associated file path.
         *
         * @return True when the path is not empty.
         */
        [[nodiscard]] bool has_path() const noexcept;

        /**
         * @brief Returns the associated file path.
         *
         * @return Read-only path reference.
         */
        [[nodiscard]] const std::filesystem::path& path() const noexcept;

        /**
         * @brief Returns a name suitable for document presentation.
         *
         * @return File name when a path exists, otherwise "Untitled".
         */
        [[nodiscard]] std::string display_name() const;

        /**
         * @brief Marks persistent document state as modified.
         */
        void mark_dirty() noexcept;

        /**
         * @brief Records a successful save and clears persistent dirty state.
         *
         * @param path Path used by the successful save operation.
         */
        void mark_saved(const std::filesystem::path& path);

        /**
         * @brief Removes the associated path without changing dirty state.
         */
        void clear_path();

    private:
        DocumentId id_{};
        editor::Editor editor_{};
        editor::CommandDispatcher commandDispatcher_;
        editor::HistoryStack history_{};
        editor::ToolRegistry toolRegistry_{};
        editor::ToolManager toolManager_;
        editor::EditorSync editorSync_{};
        std::filesystem::path path_{};
        bool dirty_ = false;
    };

} // namespace locus::application
