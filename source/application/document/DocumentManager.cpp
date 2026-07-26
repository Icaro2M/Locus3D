/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/document/DocumentManager.h"

#include <algorithm>

namespace locus::application {

    DocumentSession& DocumentManager::create_document()
    {
        const DocumentId id = next_document_id();
        auto document = std::make_unique<DocumentSession>(id);
        DocumentSession& session = *document;

        documents_.push_back(std::move(document));
        activeDocument_ = id;
        return session;
    }

    DocumentSession* DocumentManager::find(DocumentId id) noexcept
    {
        const auto it = std::find_if(
            documents_.begin(),
            documents_.end(),
            [id](const std::unique_ptr<DocumentSession>& document) {
                return document->id() == id;
            });

        return it == documents_.end() ? nullptr : it->get();
    }

    const DocumentSession*
    DocumentManager::find(DocumentId id) const noexcept
    {
        const auto it = std::find_if(
            documents_.begin(),
            documents_.end(),
            [id](const std::unique_ptr<DocumentSession>& document) {
                return document->id() == id;
            });

        return it == documents_.end() ? nullptr : it->get();
    }

    DocumentSession* DocumentManager::active_document() noexcept
    {
        return find(activeDocument_);
    }

    const DocumentSession*
    DocumentManager::active_document() const noexcept
    {
        return find(activeDocument_);
    }

    bool DocumentManager::set_active_document(DocumentId id) noexcept
    {
        if (!id.is_valid() || !find(id)) {
            return false;
        }

        activeDocument_ = id;
        return true;
    }

    bool DocumentManager::close_document(DocumentId id)
    {
        const auto it = std::find_if(
            documents_.begin(),
            documents_.end(),
            [id](const std::unique_ptr<DocumentSession>& document) {
                return document->id() == id;
            });

        if (it == documents_.end()) {
            return false;
        }

        const bool wasActive = activeDocument_ == id;
        const std::size_t index =
            static_cast<std::size_t>(
                std::distance(documents_.begin(), it));

        documents_.erase(it);

        if (!wasActive) {
            return true;
        }

        if (documents_.empty()) {
            activeDocument_ = {};
            return true;
        }

        const std::size_t replacementIndex =
            std::min(index, documents_.size() - 1);
        activeDocument_ = documents_[replacementIndex]->id();
        return true;
    }

    std::size_t DocumentManager::document_count() const noexcept
    {
        return documents_.size();
    }

    bool DocumentManager::empty() const noexcept
    {
        return documents_.empty();
    }

    DocumentId DocumentManager::next_document_id() noexcept
    {
        if (nextId_ == InvalidDocumentIdValue) {
            nextId_ = 0;
        }

        DocumentId candidate{ nextId_++ };

        while (find(candidate)) {
            if (nextId_ == InvalidDocumentIdValue) {
                nextId_ = 0;
            }

            candidate = DocumentId{ nextId_++ };
        }

        return candidate;
    }

} // namespace locus::application
