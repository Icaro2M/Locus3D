/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "application/document/DocumentId.h"
#include "application/document/DocumentSession.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace locus::application {

    /**
     * @brief Owns open document sessions and tracks the active document.
     *
     * File IO, save confirmation, dialogs, and rendering are intentionally
     * outside this manager.
     */
    class DocumentManager {
    public:
        /**
         * @brief Creates and activates an empty document session.
         *
         * @return Reference to the newly owned session.
         */
        [[nodiscard]] DocumentSession& create_document();

        /**
         * @brief Finds a document by identifier.
         *
         * @param id Document identifier.
         * @return Mutable session pointer, or null when not found.
         */
        [[nodiscard]] DocumentSession* find(DocumentId id) noexcept;

        /**
         * @brief Finds a document by identifier.
         *
         * @param id Document identifier.
         * @return Read-only session pointer, or null when not found.
         */
        [[nodiscard]] const DocumentSession*
            find(DocumentId id) const noexcept;

        /**
         * @brief Returns the currently active document.
         *
         * @return Mutable active session pointer, or null when empty.
         */
        [[nodiscard]] DocumentSession* active_document() noexcept;

        /**
         * @brief Returns the currently active document.
         *
         * @return Read-only active session pointer, or null when empty.
         */
        [[nodiscard]] const DocumentSession*
            active_document() const noexcept;

        /**
         * @brief Makes an owned document active.
         *
         * @param id Identifier of the document to activate.
         * @return True when the document exists and became active.
         */
        [[nodiscard]] bool set_active_document(DocumentId id) noexcept;

        /**
         * @brief Closes and destroys an owned document session.
         *
         * No save confirmation is performed. When the active document is
         * removed, another remaining session is selected.
         *
         * @param id Identifier of the document to close.
         * @return True when a document was removed.
         */
        [[nodiscard]] bool close_document(DocumentId id);

        /**
         * @brief Returns the number of open documents.
         *
         * @return Owned document count.
         */
        [[nodiscard]] std::size_t document_count() const noexcept;

        /**
         * @brief Checks whether no documents are open.
         *
         * @return True when the manager owns no sessions.
         */
        [[nodiscard]] bool empty() const noexcept;

    private:
        [[nodiscard]] DocumentId next_document_id() noexcept;

        std::vector<std::unique_ptr<DocumentSession>> documents_{};
        DocumentId activeDocument_{};
        DocumentIdValue nextId_ = 0;
    };

} // namespace locus::application
