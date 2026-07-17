/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/actions/core/ActionCategory.h"
#include "editor/actions/core/ActionContext.h"
#include "editor/actions/core/ActionDescriptor.h"
#include "editor/actions/core/ActionId.h"
#include "editor/actions/core/ActionResult.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/RenameNodeCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/SceneNode.h"

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace {

    using namespace locus::editor;

    void print_result(
        bool condition,
        const std::string& message) {
        std::cout
            << (condition ? "[OK] " : "[FAIL] ")
            << message
            << '\n';
    }

    const char* action_result_code_name(
        ActionResultCode code) {
        switch (code) {
        case ActionResultCode::Executed:
            return "Executed";

        case ActionResultCode::Unavailable:
            return "Unavailable";

        case ActionResultCode::Failed:
            return "Failed";
        }

        return "Unknown";
    }

    void print_action_result(
        const std::string& label,
        const ActionResult& result) {
        std::cout << label << '\n';
        std::cout
            << "  code: "
            << action_result_code_name(result.code)
            << '\n';
        std::cout
            << "  succeeded: "
            << (result.succeeded() ? "true" : "false")
            << '\n';
        std::cout
            << "  unavailable: "
            << (result.is_unavailable() ? "true" : "false")
            << '\n';
        std::cout
            << "  failed: "
            << (result.failed() ? "true" : "false")
            << '\n';
        std::cout
            << "  message: "
            << result.message
            << '\n';
    }

    bool test_action_id() {
        std::cout << "\n=== ActionId ===\n";

        const ActionId invalid{};
        const ActionId flipFace{ "mesh.face.flip" };
        const ActionId sameFlipFace{ "mesh.face.flip" };
        const ActionId deleteSelected{ "selection.delete" };

        print_result(
            invalid.is_invalid(),
            "identificador vazio e invalido");

        print_result(
            flipFace.is_valid(),
            "identificador textual e valido");

        print_result(
            flipFace == sameFlipFace,
            "identificadores com o mesmo valor sao iguais");

        print_result(
            flipFace != deleteSelected,
            "identificadores diferentes nao sao iguais");

        std::unordered_map<ActionId, std::string> names{};
        names.emplace(flipFace, "Flip Face");
        names.emplace(deleteSelected, "Delete Selected");

        print_result(
            names.size() == 2u,
            "ActionId pode ser usado em unordered_map");

        print_result(
            names.at(ActionId{ "mesh.face.flip" }) == "Flip Face",
            "hash encontra identificador equivalente");

        return invalid.is_invalid()
            && flipFace.is_valid()
            && flipFace == sameFlipFace
            && flipFace != deleteSelected
            && names.size() == 2u;
    }

    bool test_action_descriptor() {
        std::cout << "\n=== ActionDescriptor ===\n";

        const ActionDescriptor empty{};

        const ActionDescriptor descriptor{
            ActionId{ "mesh.face.flip" },
            "Flip Face",
            "Reverses the orientation of selected mesh faces.",
            ActionCategory::Mesh,
            {
                "normal",
                "orientation",
                "reverse"
            }
        };

        print_result(
            !empty.is_valid(),
            "descritor vazio e invalido");

        print_result(
            descriptor.is_valid(),
            "descritor completo e valido");

        print_result(
            descriptor.id == ActionId{ "mesh.face.flip" },
            "descritor preserva identificador");

        print_result(
            descriptor.name == "Flip Face",
            "descritor preserva nome");

        print_result(
            descriptor.category == ActionCategory::Mesh,
            "descritor preserva categoria");

        print_result(
            descriptor.keywords.size() == 3u,
            "descritor preserva palavras de busca");

        return !empty.is_valid()
            && descriptor.is_valid()
            && descriptor.id == ActionId{ "mesh.face.flip" }
            && descriptor.name == "Flip Face"
            && descriptor.category == ActionCategory::Mesh
            && descriptor.keywords.size() == 3u;
    }

    bool test_action_result() {
        std::cout << "\n=== ActionResult ===\n";

        const ActionResult executed = ActionResult::executed(
            EditorDirtyFlags::Scene,
            "Action executed.");

        const ActionResult unavailable =
            ActionResult::unavailable(
                "No compatible selection.");

        const ActionResult failed =
            ActionResult::fail(
                "Kernel operation failed.");

        print_action_result("executed result", executed);
        print_action_result("unavailable result", unavailable);
        print_action_result("failed result", failed);

        print_result(
            executed.succeeded()
            && static_cast<bool>(executed),
            "resultado executado representa sucesso");

        print_result(
            has_flag(
                executed.dirtyFlags,
                EditorDirtyFlags::Scene),
            "resultado executado preserva dirty flags");

        print_result(
            unavailable.is_unavailable()
            && !static_cast<bool>(unavailable),
            "resultado indisponivel nao representa sucesso");

        print_result(
            failed.failed()
            && !static_cast<bool>(failed),
            "resultado de falha e reconhecido");

        const ActionResult convertedSuccess =
            ActionResult::from_command(
                CommandResult::ok(
                    EditorDirtyFlags::Selection,
                    "Command completed."));

        const ActionResult convertedFailure =
            ActionResult::from_command(
                CommandResult::fail(
                    "Command rejected.",
                    EditorDirtyFlags::Mesh));

        print_result(
            convertedSuccess.succeeded()
            && has_flag(
                convertedSuccess.dirtyFlags,
                EditorDirtyFlags::Selection)
            && convertedSuccess.message
            == "Command completed.",
            "conversao preserva command de sucesso");

        print_result(
            convertedFailure.failed()
            && has_flag(
                convertedFailure.dirtyFlags,
                EditorDirtyFlags::Mesh)
            && convertedFailure.message
            == "Command rejected.",
            "conversao preserva command de falha");

        return executed.succeeded()
            && unavailable.is_unavailable()
            && failed.failed()
            && convertedSuccess.succeeded()
            && convertedFailure.failed();
    }

    bool test_action_context() {
        std::cout << "\n=== ActionContext ===\n";

        Editor editor{};
        editor.set_mode(EditorMode::Object);
        editor.clear_dirty();

        CommandDispatcher dispatcher{ editor };
        HistoryStack history{};

        ActionContext context{
            editor,
            dispatcher,
            history
        };

        print_result(
            &context.editor() == &editor,
            "contexto retorna o Editor original");

        print_result(
            &context.state() == &editor.state(),
            "contexto retorna o EditorState original");

        print_result(
            &context.scene() == &editor.scene(),
            "contexto retorna a EditorScene original");

        print_result(
            &context.selection() == &editor.selection(),
            "contexto retorna a SelectionState original");

        print_result(
            &context.selection_controller()
            == &editor.selection_controller(),
            "contexto retorna o SelectionController original");

        print_result(
            context.mode() == EditorMode::Object,
            "contexto retorna o modo atual");

        print_result(
            &context.dispatcher() == &dispatcher,
            "contexto retorna o dispatcher original");

        print_result(
            &context.history() == &history,
            "contexto retorna o historico original");

        context.mark_dirty(EditorDirtyFlags::Selection);

        print_result(
            has_flag(
                editor.dirty_flags(),
                EditorDirtyFlags::Selection),
            "mark_dirty encaminha flags ao Editor");

        editor.clear_dirty();

        const SceneNodeId nodeId =
            editor.scene().create_empty("Original Name");

        SceneNode* node =
            editor.scene().find_node(nodeId);

        print_result(
            node != nullptr,
            "node de teste foi criado");

        if (!node) {
            return false;
        }

        print_result(
            node->metadata().name == "Original Name",
            "node inicia com o nome esperado");

        const CommandResult renameResult =
            context.execute_command(
                std::make_unique<RenameNodeCommand>(
                    nodeId,
                    "Renamed By Action"));

        std::cout << "rename command\n";
        std::cout
            << "  success: "
            << (renameResult.success ? "true" : "false")
            << '\n';
        std::cout
            << "  message: "
            << renameResult.message
            << '\n';

        print_result(
            renameResult.success,
            "ActionContext executou command");

        print_result(
            node->metadata().name == "Renamed By Action",
            "command alterou o nome do node");

        print_result(
            history.undo_size() == 1u
            && history.can_undo(),
            "command entrou no historico");

        const CommandResult undoResult =
            history.undo(dispatcher);

        print_result(
            undoResult.success,
            "undo do command funcionou");

        print_result(
            node->metadata().name == "Original Name",
            "undo restaurou o nome original");

        print_result(
            history.can_redo(),
            "redo ficou disponivel");

        const CommandResult redoResult =
            history.redo(dispatcher);

        print_result(
            redoResult.success,
            "redo do command funcionou");

        print_result(
            node->metadata().name == "Renamed By Action",
            "redo reaplicou o novo nome");

        const CommandResult emptyResult =
            context.execute_command(nullptr);

        print_result(
            !emptyResult.success,
            "contexto rejeita command vazio");

        print_result(
            emptyResult.message
            == "Cannot execute an empty action command.",
            "command vazio retorna mensagem esperada");

        return renameResult.success
            && undoResult.success
            && redoResult.success
            && node->metadata().name == "Renamed By Action"
            && history.undo_size() == 1u
            && !emptyResult.success;
    }

} // namespace

int main() {
    std::cout
        << "=== Locus3D Editor Actions Core Smoke Test ===\n";

    bool passed = true;

    passed = test_action_id() && passed;
    passed = test_action_descriptor() && passed;
    passed = test_action_result() && passed;
    passed = test_action_context() && passed;

    std::cout << '\n';

    if (passed) {
        std::cout
            << "=== All actions core smoke tests passed ===\n";
        return 0;
    }

    std::cout
        << "=== Actions core smoke test failed ===\n";
    return 1;
}