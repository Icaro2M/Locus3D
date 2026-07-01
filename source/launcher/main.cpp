/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/CommandResult.h"
#include "editor/command/selection/SetSelectionGranularityCommand.h"
#include "editor/command/selection/SetSelectionScopeCommand.h"
#include "editor/command/selection/ToggleObjectSelectionCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/SceneNode.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

    using namespace locus::editor;

    struct TestStats {
        int passed = 0;
        int failed = 0;
    };

    void expect(TestStats& stats, bool condition, const std::string& message)
    {
        if (condition) {
            ++stats.passed;
            std::cout << "[OK] " << message << '\n';
        }
        else {
            ++stats.failed;
            std::cout << "[FAIL] " << message << '\n';
        }
    }

    void print_result(const std::string& label, const CommandResult& result)
    {
        std::cout << label << '\n';
        std::cout << "  success: " << (result.success ? "true" : "false") << '\n';

        if (!result.message.empty()) {
            std::cout << "  message: " << result.message << '\n';
        }
    }

    const char* granularity_name(SelectionGranularity granularity)
    {
        switch (granularity) {
        case SelectionGranularity::Object:
            return "Object";
        case SelectionGranularity::Vertex:
            return "Vertex";
        case SelectionGranularity::Edge:
            return "Edge";
        case SelectionGranularity::Loop:
            return "Loop";
        case SelectionGranularity::Face:
            return "Face";
        }

        return "Unknown";
    }

    const char* scope_name(SelectionScope scope)
    {
        switch (scope) {
        case SelectionScope::Scene:
            return "Scene";
        case SelectionScope::ActiveMesh:
            return "ActiveMesh";
        }

        return "Unknown";
    }

    void print_selection_mode(const Editor& editor)
    {
        std::cout << "  granularity: " << granularity_name(editor.selection().granularity()) << '\n';
        std::cout << "  scope: " << scope_name(editor.selection().scope()) << '\n';
    }

    bool is_selected(const Editor& editor, SceneNodeId id)
    {
        return editor.selection().objects().contains(id);
    }

    std::size_t selected_count(const Editor& editor)
    {
        return editor.selection().objects().size();
    }

    SceneNodeId active_object(const Editor& editor)
    {
        return editor.selection().objects().active();
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor Selection Commands Smoke Test ===\n\n";

    TestStats stats{};

    Editor editor{};
    CommandDispatcher dispatcher(editor);
    HistoryStack history{};

    const SceneNodeId cube = editor.scene().create_empty("Cube");
    const SceneNodeId sphere = editor.scene().create_empty("Sphere");
    const SceneNodeId locked = editor.scene().create_empty("Locked");

    SceneNode* lockedNode = editor.scene().find_node(locked);
    if (lockedNode) {
        lockedNode->metadata().selectable = false;
    }

    expect(stats, cube.is_valid(), "cube criado com id valido");
    expect(stats, sphere.is_valid(), "sphere criado com id valido");
    expect(stats, locked.is_valid(), "locked criado com id valido");
    expect(stats, selected_count(editor) == 0u, "selecao comeca vazia");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Object, "granularidade inicial eh Object");
    expect(stats, editor.selection().scope() == SelectionScope::Scene, "scope inicial eh Scene");
    expect(stats, history.empty(), "historico comeca vazio");

    std::cout << "\n=== Toggle em objeto vazio deve selecionar ===\n";

    CommandResult result = history.execute(
        dispatcher,
        std::make_unique<ToggleObjectSelectionCommand>(cube));

    print_result("toggle cube", result);

    expect(stats, result.success, "toggle cube executou com sucesso");
    expect(stats, is_selected(editor, cube), "cube ficou selecionado");
    expect(stats, selected_count(editor) == 1u, "selecao possui 1 objeto");
    expect(stats, active_object(editor) == cube, "cube virou objeto ativo");
    expect(stats, history.can_undo(), "historico permite undo apos primeiro toggle");
    expect(stats, history.undo_size() == 1u, "historico possui 1 entrada de undo");

    std::cout << "\n=== Toggle no mesmo objeto deve desselecionar ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleObjectSelectionCommand>(cube));

    print_result("toggle cube novamente", result);

    expect(stats, result.success, "segundo toggle cube executou com sucesso");
    expect(stats, !is_selected(editor, cube), "cube foi removido da selecao");
    expect(stats, selected_count(editor) == 0u, "selecao ficou vazia");
    expect(stats, history.undo_size() == 2u, "historico possui 2 entradas de undo");

    std::cout << "\n=== Undo do segundo toggle deve restaurar cube selecionado ===\n";

    result = history.undo(dispatcher);

    print_result("undo segundo toggle", result);

    expect(stats, result.success, "undo do segundo toggle executou com sucesso");
    expect(stats, is_selected(editor, cube), "cube voltou a ficar selecionado");
    expect(stats, selected_count(editor) == 1u, "selecao voltou a ter 1 objeto");
    expect(stats, active_object(editor) == cube, "cube voltou a ser objeto ativo");
    expect(stats, history.undo_size() == 1u, "historico voltou para 1 undo");
    expect(stats, history.redo_size() == 1u, "historico possui 1 redo");

    std::cout << "\n=== Undo do primeiro toggle deve restaurar selecao vazia ===\n";

    result = history.undo(dispatcher);

    print_result("undo primeiro toggle", result);

    expect(stats, result.success, "undo do primeiro toggle executou com sucesso");
    expect(stats, !is_selected(editor, cube), "cube nao esta mais selecionado");
    expect(stats, selected_count(editor) == 0u, "selecao voltou a ficar vazia");
    expect(stats, history.undo_size() == 0u, "historico nao possui mais undo");
    expect(stats, history.redo_size() == 2u, "historico possui 2 redos");

    std::cout << "\n=== Redo deve selecionar cube novamente ===\n";

    result = history.redo(dispatcher);

    print_result("redo primeiro toggle", result);

    expect(stats, result.success, "redo do primeiro toggle executou com sucesso");
    expect(stats, is_selected(editor, cube), "cube voltou a ficar selecionado pelo redo");
    expect(stats, selected_count(editor) == 1u, "selecao possui 1 objeto apos redo");
    expect(stats, history.undo_size() == 1u, "historico possui 1 undo apos redo");
    expect(stats, history.redo_size() == 1u, "historico possui 1 redo restante");

    std::cout << "\n=== Novo toggle deve limpar pilha de redo ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleObjectSelectionCommand>(sphere));

    print_result("toggle sphere", result);

    expect(stats, result.success, "toggle sphere executou com sucesso");
    expect(stats, is_selected(editor, cube), "cube continua selecionado");
    expect(stats, is_selected(editor, sphere), "sphere foi adicionada a selecao");
    expect(stats, selected_count(editor) == 2u, "selecao possui 2 objetos");
    expect(stats, active_object(editor) == sphere, "sphere virou objeto ativo");
    expect(stats, history.redo_size() == 0u, "novo comando limpou a pilha de redo");

    std::cout << "\n=== Toggle em objeto nao selecionavel deve falhar sem alterar selecao ===\n";

    const std::size_t countBeforeLockedToggle = selected_count(editor);
    const bool cubeSelectedBeforeLockedToggle = is_selected(editor, cube);
    const bool sphereSelectedBeforeLockedToggle = is_selected(editor, sphere);
    const std::size_t undoBeforeLockedToggle = history.undo_size();

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleObjectSelectionCommand>(locked));

    print_result("toggle locked", result);

    expect(stats, !result.success, "toggle locked falhou como esperado");
    expect(stats, selected_count(editor) == countBeforeLockedToggle, "falha nao alterou tamanho da selecao");
    expect(stats, is_selected(editor, cube) == cubeSelectedBeforeLockedToggle, "falha manteve estado do cube");
    expect(stats, is_selected(editor, sphere) == sphereSelectedBeforeLockedToggle, "falha manteve estado da sphere");
    expect(stats, history.undo_size() == undoBeforeLockedToggle, "falha nao entrou no historico");

    std::cout << "\n=== SetSelectionGranularity: Object -> Vertex ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SetSelectionGranularityCommand>(SelectionGranularity::Vertex));

    print_result("set granularity Vertex", result);
    print_selection_mode(editor);

    expect(stats, result.success, "set granularity Vertex executou com sucesso");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Vertex, "granularidade virou Vertex");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "scope virou ActiveMesh ao entrar em componente");

    std::cout << "\n=== SetSelectionGranularity: Vertex -> Face ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SetSelectionGranularityCommand>(SelectionGranularity::Face));

    print_result("set granularity Face", result);
    print_selection_mode(editor);

    expect(stats, result.success, "set granularity Face executou com sucesso");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Face, "granularidade virou Face");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "scope continuou ActiveMesh");

    std::cout << "\n=== SetSelectionScope: ActiveMesh -> Scene deve forcar Object ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SetSelectionScopeCommand>(SelectionScope::Scene));

    print_result("set scope Scene", result);
    print_selection_mode(editor);

    expect(stats, result.success, "set scope Scene executou com sucesso");
    expect(stats, editor.selection().scope() == SelectionScope::Scene, "scope virou Scene");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Object, "Scene forcou granularidade Object");

    std::cout << "\n=== Undo do SetSelectionScope deve restaurar Face + ActiveMesh ===\n";

    result = history.undo(dispatcher);

    print_result("undo set scope Scene", result);
    print_selection_mode(editor);

    expect(stats, result.success, "undo do set scope executou com sucesso");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Face, "undo restaurou granularidade Face");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "undo restaurou scope ActiveMesh");

    std::cout << "\n=== Undo do SetSelectionGranularity Face deve restaurar Vertex + ActiveMesh ===\n";

    result = history.undo(dispatcher);

    print_result("undo set granularity Face", result);
    print_selection_mode(editor);

    expect(stats, result.success, "undo do set granularity Face executou com sucesso");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Vertex, "undo restaurou granularidade Vertex");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "undo manteve/restaurou scope ActiveMesh");

    std::cout << "\n=== Undo do SetSelectionGranularity Vertex deve restaurar Object + Scene ===\n";

    result = history.undo(dispatcher);

    print_result("undo set granularity Vertex", result);
    print_selection_mode(editor);

    expect(stats, result.success, "undo do set granularity Vertex executou com sucesso");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Object, "undo restaurou granularidade Object");
    expect(stats, editor.selection().scope() == SelectionScope::Scene, "undo restaurou scope Scene");

    std::cout << "\n=== Redo deve reaplicar Vertex + ActiveMesh ===\n";

    result = history.redo(dispatcher);

    print_result("redo set granularity Vertex", result);
    print_selection_mode(editor);

    expect(stats, result.success, "redo do set granularity Vertex executou com sucesso");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Vertex, "redo reaplicou granularidade Vertex");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "redo reaplicou scope ActiveMesh");

    std::cout << "\n=== SetSelectionGranularity para Object deve voltar para Scene ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SetSelectionGranularityCommand>(SelectionGranularity::Object));

    print_result("set granularity Object", result);
    print_selection_mode(editor);

    expect(stats, result.success, "set granularity Object executou com sucesso");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Object, "granularidade virou Object");
    expect(stats, editor.selection().scope() == SelectionScope::Scene, "scope virou Scene ao voltar para Object");

    std::cout << "\n=== SetSelectionScope ActiveMesh a partir de Object ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SetSelectionScopeCommand>(SelectionScope::ActiveMesh));

    print_result("set scope ActiveMesh", result);
    print_selection_mode(editor);

    expect(stats, result.success, "set scope ActiveMesh executou com sucesso");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "scope virou ActiveMesh");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Object, "granularidade continuou Object");

    std::cout << "\n=== SetSelectionScope Scene deve manter Object ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SetSelectionScopeCommand>(SelectionScope::Scene));

    print_result("set scope Scene novamente", result);
    print_selection_mode(editor);

    expect(stats, result.success, "set scope Scene novamente executou com sucesso");
    expect(stats, editor.selection().scope() == SelectionScope::Scene, "scope voltou para Scene");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Object, "granularidade continuou Object");

    std::cout << "\n=== Resultado final ===\n";
    std::cout << "passed: " << stats.passed << '\n';
    std::cout << "failed: " << stats.failed << '\n';

    if (stats.failed == 0) {
        std::cout << "\n[OK] Editor selection commands smoke test passou.\n";
        return 0;
    }

    std::cout << "\n[FAIL] Editor selection commands smoke test encontrou problemas.\n";
    return 1;
}