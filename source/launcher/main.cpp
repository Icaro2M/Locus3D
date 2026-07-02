/*
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/scene/DeleteNodeCommand.h"
#include "editor/command/scene/DuplicateNodeCommand.h"
#include "editor/command/scene/ReparentNodeCommand.h"
#include "editor/scene/SceneNode.h"
#include "editor/scene/SceneNodeId.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

    using namespace locus::editor;

    int g_failures = 0;

    void expect(bool condition, std::string_view message) {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return;
        }

        std::cout << "[FAIL] " << message << '\n';
        ++g_failures;
    }

    void print_result(std::string_view label, const CommandResult& result) {
        std::cout << label << '\n';
        std::cout << "  success: " << (result.success ? "true" : "false") << '\n';
        std::cout << "  message: " << result.message << '\n';
    }

    std::string node_name(const Editor& editor, SceneNodeId id) {
        const SceneNode* node = editor.scene().find_node(id);
        if (!node) {
            return "<missing>";
        }

        return node->metadata().name;
    }

    SceneNodeId find_child_by_name(const Editor& editor, SceneNodeId parent, std::string_view name) {
        const SceneNode* parentNode = editor.scene().find_node(parent);
        if (!parentNode) {
            return {};
        }

        for (SceneNodeId child : parentNode->children()) {
            const SceneNode* childNode = editor.scene().find_node(child);
            if (childNode && childNode->metadata().name == name) {
                return child;
            }
        }

        return {};
    }

    void print_scene_node_recursive(const Editor& editor, SceneNodeId id, int depth = 0) {
        const SceneNode* node = editor.scene().find_node(id);
        if (!node) {
            return;
        }

        for (int i = 0; i < depth; ++i) {
            std::cout << "  ";
        }

        std::cout
            << "- id=" << node->id().value
            << " name=\"" << node->metadata().name << "\""
            << " parent=";

        if (node->parent().is_valid()) {
            std::cout << node->parent().value;
        }
        else {
            std::cout << "root";
        }

        std::cout << " children=" << node->children().size() << '\n';

        for (SceneNodeId child : node->children()) {
            print_scene_node_recursive(editor, child, depth + 1);
        }
    }

    void print_scene(const Editor& editor) {
        std::cout << "\nScene tree:\n";

        const auto& roots = editor.scene().tree().roots();
        if (roots.empty()) {
            std::cout << "  <empty>\n";
            return;
        }

        for (SceneNodeId root : roots) {
            print_scene_node_recursive(editor, root, 1);
        }
    }

    void test_reparent_command() {
        std::cout << "\n=== ReparentNodeCommand smoke test ===\n";

        Editor editor;
        CommandDispatcher dispatcher(editor);

        const SceneNodeId rootA = editor.scene().create_empty("Root A");
        const SceneNodeId rootB = editor.scene().create_empty("Root B");
        const SceneNodeId child = editor.scene().create_mesh("Child Mesh");

        expect(editor.scene().tree().size() == 3, "cena inicial tem 3 nos");
        expect(editor.scene().find_node(child)->parent().is_invalid(), "child comeca como root");

        ReparentNodeCommand reparent(child, rootA);
        CommandResult result = dispatcher.execute(reparent);
        print_result("reparent child -> rootA", result);

        expect(result.success, "reparent executou com sucesso");
        expect(editor.scene().find_node(child)->parent() == rootA, "child virou filho de rootA");
        expect(editor.scene().find_node(rootA)->children().size() == 1, "rootA recebeu 1 filho");

        result = dispatcher.undo(reparent);
        print_result("undo reparent", result);

        expect(result.success, "undo do reparent executou com sucesso");
        expect(editor.scene().find_node(child)->parent().is_invalid(), "child voltou para root");
        expect(editor.scene().find_node(rootA)->children().empty(), "rootA voltou a ficar sem filhos");

        result = dispatcher.redo(reparent);
        print_result("redo reparent", result);

        expect(result.success, "redo do reparent executou com sucesso");
        expect(editor.scene().find_node(child)->parent() == rootA, "child voltou para rootA");

        ReparentNodeCommand makeRoot(child, {});
        result = dispatcher.execute(makeRoot);
        print_result("reparent child -> root", result);

        expect(result.success, "reparent para root executou com sucesso");
        expect(editor.scene().find_node(child)->parent().is_invalid(), "child virou root novamente");

        ReparentNodeCommand invalidSelf(rootB, rootB);
        result = dispatcher.execute(invalidSelf);
        print_result("falha self parent", result);

        expect(!result.success, "self parent falhou corretamente");

        ReparentNodeCommand parentAUnderChild(rootA, child);
        result = dispatcher.execute(parentAUnderChild);
        print_result("rootA -> child", result);

        expect(result.success, "rootA pode virar filho de child quando nao ha ciclo");
        expect(editor.scene().find_node(rootA)->parent() == child, "rootA virou filho de child");

        ReparentNodeCommand cycle(child, rootA);
        result = dispatcher.execute(cycle);
        print_result("falha ciclo child -> rootA descendente", result);

        expect(!result.success, "reparent que criaria ciclo falhou corretamente");

        print_scene(editor);
    }

    void test_delete_command() {
        std::cout << "\n=== DeleteNodeCommand smoke test ===\n";

        Editor editor;
        CommandDispatcher dispatcher(editor);

        const SceneNodeId root = editor.scene().create_empty("Root");
        const SceneNodeId childA = editor.scene().create_mesh("Child A");
        const SceneNodeId childB = editor.scene().create_empty("Child B");
        const SceneNodeId grandChild = editor.scene().create_mesh("Grand Child");
        const SceneNodeId survivor = editor.scene().create_empty("Survivor");

        editor.scene().reparent(childA, root);
        editor.scene().reparent(childB, root);
        editor.scene().reparent(grandChild, childA);

        editor.selection().objects().set({ childA, survivor }, childA);
        editor.selection().mesh().set_active_mesh(childA);
        editor.selection().clear_dirty();

        expect(editor.scene().tree().size() == 5, "cena inicial tem 5 nos");
        expect(editor.selection().objects().contains(childA), "childA esta selecionado antes do delete");
        expect(editor.selection().objects().contains(survivor), "survivor esta selecionado antes do delete");
        expect(editor.selection().mesh().active_mesh() == childA, "active mesh aponta para childA antes do delete");

        DeleteNodeCommand deleteRoot(root);
        CommandResult result = dispatcher.execute(deleteRoot);
        print_result("delete root subtree", result);

        expect(result.success, "delete executou com sucesso");
        expect(editor.scene().tree().size() == 1, "delete removeu root e descendentes");
        expect(!editor.scene().find_node(root), "root foi removido");
        expect(!editor.scene().find_node(childA), "childA foi removido");
        expect(!editor.scene().find_node(childB), "childB foi removido");
        expect(!editor.scene().find_node(grandChild), "grandChild foi removido");
        expect(editor.scene().find_node(survivor) != nullptr, "survivor permaneceu na cena");

        expect(!editor.selection().objects().contains(childA), "delete removeu childA da selecao");
        expect(editor.selection().objects().contains(survivor), "delete preservou survivor selecionado");
        expect(editor.selection().objects().active().is_invalid(), "active object removido foi limpo");
        expect(editor.selection().mesh().active_mesh().is_invalid(), "active mesh removido foi limpo");

        result = dispatcher.undo(deleteRoot);
        print_result("undo delete", result);

        expect(result.success, "undo do delete executou com sucesso");
        expect(editor.scene().tree().size() == 5, "undo restaurou todos os nos");
        expect(editor.scene().find_node(root) != nullptr, "root foi restaurado");
        expect(editor.scene().find_node(childA) != nullptr, "childA foi restaurado");
        expect(editor.scene().find_node(childB) != nullptr, "childB foi restaurado");
        expect(editor.scene().find_node(grandChild) != nullptr, "grandChild foi restaurado");
        expect(editor.scene().find_node(childA)->parent() == root, "childA voltou como filho de root");
        expect(editor.scene().find_node(childB)->parent() == root, "childB voltou como filho de root");
        expect(editor.scene().find_node(grandChild)->parent() == childA, "grandChild voltou como filho de childA");
        expect(node_name(editor, root) == "Root", "nome do root foi preservado");
        expect(node_name(editor, childA) == "Child A", "nome do childA foi preservado");

        result = dispatcher.redo(deleteRoot);
        print_result("redo delete", result);

        expect(result.success, "redo do delete executou com sucesso");
        expect(editor.scene().tree().size() == 1, "redo removeu subtree de novo");
        expect(editor.scene().find_node(survivor) != nullptr, "survivor ainda existe depois do redo");

        DeleteNodeCommand deleteMissing(root);
        result = dispatcher.execute(deleteMissing);
        print_result("falha delete missing", result);

        expect(!result.success, "delete de no ausente falhou corretamente");

        print_scene(editor);
    }

    void test_duplicate_command() {
        std::cout << "\n=== DuplicateNodeCommand smoke test ===\n";

        Editor editor;
        CommandDispatcher dispatcher(editor);

        const SceneNodeId root = editor.scene().create_empty("Root");
        const SceneNodeId childA = editor.scene().create_mesh("Child A");
        const SceneNodeId childB = editor.scene().create_empty("Child B");
        const SceneNodeId grandChild = editor.scene().create_mesh("Grand Child");
        const SceneNodeId externalParent = editor.scene().create_empty("External Parent");

        editor.scene().reparent(root, externalParent);
        editor.scene().reparent(childA, root);
        editor.scene().reparent(childB, root);
        editor.scene().reparent(grandChild, childA);

        expect(editor.scene().tree().size() == 5, "cena inicial tem 5 nos");
        expect(editor.scene().find_node(root)->parent() == externalParent, "root original esta sob externalParent");

        DuplicateNodeCommand duplicateRoot(root);
        CommandResult result = dispatcher.execute(duplicateRoot);
        print_result("duplicate root subtree", result);

        const SceneNodeId duplicatedRoot = duplicateRoot.duplicated_node();

        expect(result.success, "duplicate executou com sucesso");
        expect(duplicatedRoot.is_valid(), "duplicate retornou id valido");
        expect(duplicatedRoot != root, "duplicado tem id diferente do original");
        expect(editor.scene().tree().size() == 9, "duplicate criou 4 novos nos");
        expect(editor.scene().find_node(duplicatedRoot) != nullptr, "root duplicado existe");
        expect(editor.scene().find_node(duplicatedRoot)->parent() == externalParent, "root duplicado manteve parent externo");
        expect(node_name(editor, duplicatedRoot) == "Root Copy", "root duplicado recebeu sufixo Copy");

        const SceneNodeId duplicatedChildA = find_child_by_name(editor, duplicatedRoot, "Child A");
        const SceneNodeId duplicatedChildB = find_child_by_name(editor, duplicatedRoot, "Child B");

        expect(duplicatedChildA.is_valid(), "childA duplicado foi encontrado por nome");
        expect(duplicatedChildB.is_valid(), "childB duplicado foi encontrado por nome");
        expect(duplicatedChildA != childA, "childA duplicado tem id diferente");
        expect(duplicatedChildB != childB, "childB duplicado tem id diferente");

        const SceneNodeId duplicatedGrandChild = find_child_by_name(editor, duplicatedChildA, "Grand Child");
        expect(duplicatedGrandChild.is_valid(), "grandChild duplicado foi encontrado");
        expect(duplicatedGrandChild != grandChild, "grandChild duplicado tem id diferente");
        expect(editor.scene().find_node(duplicatedGrandChild)->parent() == duplicatedChildA, "hierarquia profunda foi preservada");

        result = dispatcher.undo(duplicateRoot);
        print_result("undo duplicate", result);

        expect(result.success, "undo do duplicate executou com sucesso");
        expect(editor.scene().tree().size() == 5, "undo removeu os nos duplicados");
        expect(!editor.scene().find_node(duplicatedRoot), "root duplicado foi removido no undo");
        expect(editor.scene().find_node(root) != nullptr, "root original permaneceu no undo");

        result = dispatcher.redo(duplicateRoot);
        print_result("redo duplicate", result);

        expect(result.success, "redo do duplicate executou com sucesso");
        expect(editor.scene().tree().size() == 9, "redo restaurou os nos duplicados");
        expect(editor.scene().find_node(duplicatedRoot) != nullptr, "root duplicado foi restaurado com mesmo id");
        expect(editor.scene().find_node(duplicatedRoot)->parent() == externalParent, "parent externo do duplicado foi preservado no redo");

        const SceneNodeId redoChildA = find_child_by_name(editor, duplicatedRoot, "Child A");
        const SceneNodeId redoGrandChild = find_child_by_name(editor, redoChildA, "Grand Child");

        expect(redoChildA == duplicatedChildA, "redo preservou id do childA duplicado");
        expect(redoGrandChild == duplicatedGrandChild, "redo preservou id do grandChild duplicado");

        DuplicateNodeCommand duplicateMissing(SceneNodeId{ 999999 });
        result = dispatcher.execute(duplicateMissing);
        print_result("falha duplicate missing", result);

        expect(!result.success, "duplicate de no ausente falhou corretamente");

        print_scene(editor);
    }

}

int main() {
    std::cout << "=== Locus3D Editor Scene Commands Regression Test ===\n";

    test_reparent_command();
    test_delete_command();
    test_duplicate_command();

    std::cout << "\n=== Resultado final ===\n";

    if (g_failures == 0) {
        std::cout << "[OK] todos os testes passaram\n";
        return EXIT_SUCCESS;
    }

    std::cout << "[FAIL] falhas encontradas: " << g_failures << '\n';
    return EXIT_FAILURE;
}