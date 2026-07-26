/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "application/Application.h"

#include <iostream>
#include <string_view>

namespace {

    using namespace locus::application;

    void print_result(bool condition, std::string_view message)
    {
        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

} // namespace

int main()
{
    std::cout
        << "=== Locus3D DocumentManager Headless Smoke Test ===\n";

    bool passed = true;
    DocumentManager manager{};

    const bool startsEmpty =
        manager.empty()
        && manager.document_count() == 0
        && manager.active_document() == nullptr
        && manager.find(DocumentId{}) == nullptr;
    print_result(
        startsEmpty,
        "manager starts empty without an active document");
    passed = startsEmpty && passed;

    DocumentSession& first = manager.create_document();
    const DocumentId firstId = first.id();
    const bool firstCreationIsValid =
        firstId.is_valid()
        && manager.document_count() == 1
        && manager.find(firstId) == &first
        && manager.active_document() == &first;
    print_result(
        firstCreationIsValid,
        "first document receives an ID and becomes active");
    passed = firstCreationIsValid && passed;

    DocumentSession& second = manager.create_document();
    const DocumentId secondId = second.id();
    DocumentSession& third = manager.create_document();
    const DocumentId thirdId = third.id();

    const bool multipleDocumentsAreOwned =
        manager.document_count() == 3
        && firstId != secondId
        && firstId != thirdId
        && secondId != thirdId
        && manager.find(firstId) == &first
        && manager.find(secondId) == &second
        && manager.find(thirdId) == &third
        && manager.active_document() == &third;
    print_result(
        multipleDocumentsAreOwned,
        "multiple sessions receive unique IDs and stable ownership");
    passed = multipleDocumentsAreOwned && passed;

    first.editor().scene().create_empty("First document node");
    const bool editorsRemainIsolated =
        first.editor().scene().tree().size() == 1
        && second.editor().scene().tree().size() == 0
        && third.editor().scene().tree().size() == 0;
    print_result(
        editorsRemainIsolated,
        "manager-owned sessions retain isolated editor state");
    passed = editorsRemainIsolated && passed;

    const DocumentManager& constManager = manager;
    const bool constLookupWorks =
        constManager.find(firstId) == &first
        && constManager.active_document() == &third;
    print_result(
        constLookupWorks,
        "const lookup and active-document access work");
    passed = constLookupWorks && passed;

    const bool activationWorks =
        manager.set_active_document(firstId)
        && manager.active_document() == &first
        && !manager.set_active_document(DocumentId{ 9999 })
        && !manager.set_active_document(DocumentId{})
        && manager.active_document() == &first;
    print_result(
        activationWorks,
        "only owned valid documents can become active");
    passed = activationWorks && passed;

    const bool nonActiveCloseWorks =
        manager.close_document(secondId)
        && manager.document_count() == 2
        && manager.find(secondId) == nullptr
        && manager.active_document() == &first;
    print_result(
        nonActiveCloseWorks,
        "closing an inactive document preserves the active document");
    passed = nonActiveCloseWorks && passed;

    const bool activeCloseSelectsReplacement =
        manager.close_document(firstId)
        && manager.document_count() == 1
        && manager.find(firstId) == nullptr
        && manager.active_document() == &third;
    print_result(
        activeCloseSelectsReplacement,
        "closing the active document selects a valid replacement");
    passed = activeCloseSelectsReplacement && passed;

    DocumentSession& fourth = manager.create_document();
    const DocumentId fourthId = fourth.id();
    fourth.mark_dirty();

    const bool idsAreNotReused =
        fourthId != firstId
        && fourthId != secondId
        && fourthId != thirdId;
    const bool dirtyCloseHasNoPolicy =
        manager.close_document(fourthId)
        && manager.active_document() == &third;
    print_result(
        idsAreNotReused,
        "new sessions continue the monotonic ID sequence");
    print_result(
        dirtyCloseHasNoPolicy,
        "manager closes dirty sessions without UI policy");
    passed = idsAreNotReused && dirtyCloseHasNoPolicy && passed;

    const bool finalCloseWorks =
        manager.close_document(thirdId)
        && manager.empty()
        && manager.document_count() == 0
        && manager.active_document() == nullptr
        && !manager.close_document(thirdId);
    print_result(
        finalCloseWorks,
        "closing the final document clears active state");
    passed = finalCloseWorks && passed;

    print_result(
        true,
        "manager and sessions run without a window or graphics context");

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All DocumentManager headless smoke tests passed ===\n";
        return 0;
    }

    std::cout
        << "=== DocumentManager headless smoke test failed ===\n";
    return 1;
}
