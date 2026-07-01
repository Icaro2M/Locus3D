/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/CommandResult.h"
#include "editor/command/selection/ClearMeshSelectionCommand.h"
#include "editor/command/selection/SelectMeshComponentCommand.h"
#include "editor/command/selection/ToggleMeshComponentSelectionCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/SceneNodeId.h"
#include "editor/selection/SelectionGranularity.h"
#include "editor/selection/SelectionScope.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

    using namespace locus::editor;
    namespace geometry = locus::kernel::geometry;

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

    std::size_t mesh_selection_count(const Editor& editor)
    {
        return editor.selection().mesh().vertices().size()
            + editor.selection().mesh().edges().size()
            + editor.selection().mesh().loops().size()
            + editor.selection().mesh().faces().size();
    }

    void print_mesh_selection(const Editor& editor)
    {
        const auto& mesh = editor.selection().mesh();

        std::cout << "  active mesh valid: "
            << (mesh.active_mesh().is_valid() ? "true" : "false") << '\n';
        std::cout << "  vertices: " << mesh.vertices().size() << '\n';
        std::cout << "  edges: " << mesh.edges().size() << '\n';
        std::cout << "  loops: " << mesh.loops().size() << '\n';
        std::cout << "  faces: " << mesh.faces().size() << '\n';
        std::cout << "  total: " << mesh_selection_count(editor) << '\n';
        std::cout << "  granularity: " << granularity_name(editor.selection().granularity()) << '\n';
        std::cout << "  scope: " << scope_name(editor.selection().scope()) << '\n';
        std::cout << "  empty: " << (mesh.empty() ? "true" : "false") << '\n';
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor Mesh Component Selection Commands Smoke Test ===\n\n";

    TestStats stats{};

    Editor editor{};
    CommandDispatcher dispatcher(editor);
    HistoryStack history{};

    const SceneNodeId meshNode = editor.scene().create_mesh("Editable Mesh");

    const geometry::VertexHandle v0{ 1 };
    const geometry::VertexHandle v1{ 2 };
    const geometry::VertexHandle v2{ 3 };
    const geometry::EdgeHandle e0{ 4 };
    const geometry::EdgeHandle e1{ 5 };
    const geometry::LoopHandle l0{ 6 };
    const geometry::FaceHandle f0{ 7 };

    expect(stats, meshNode.is_valid(), "mesh node criado com id valido");
    expect(stats, editor.scene().find_mesh(meshNode) != nullptr, "mesh node pode ser encontrado como MeshNode");
    expect(stats, editor.selection().mesh().empty(), "selecao de mesh comeca vazia");
    expect(stats, history.empty(), "historico comeca vazio");

    std::cout << "\n=== Falha sem active mesh ===\n";

    CommandResult result = history.execute(
        dispatcher,
        std::make_unique<SelectMeshComponentCommand>(v0));

    print_result("select vertex sem active mesh", result);
    print_mesh_selection(editor);

    expect(stats, !result.success, "select vertex sem active mesh falhou");
    expect(stats, editor.selection().mesh().empty(), "falha sem active mesh nao alterou selecao");
    expect(stats, history.empty(), "falha sem active mesh nao entrou no historico");

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleMeshComponentSelectionCommand>(v0));

    print_result("toggle vertex sem active mesh", result);
    print_mesh_selection(editor);

    expect(stats, !result.success, "toggle vertex sem active mesh falhou");
    expect(stats, editor.selection().mesh().empty(), "toggle sem active mesh nao alterou selecao");
    expect(stats, history.empty(), "toggle sem active mesh nao entrou no historico");

    std::cout << "\n=== Preparando active mesh ===\n";

    editor.selection().mesh().set_active_mesh(meshNode);
    editor.selection().set_granularity(SelectionGranularity::Vertex);
    editor.selection().set_scope(SelectionScope::ActiveMesh);

    print_mesh_selection(editor);

    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "active mesh foi definido");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Vertex, "granularidade preparada como Vertex");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "scope preparado como ActiveMesh");

    std::cout << "\n=== Falha com handle invalido ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SelectMeshComponentCommand>(geometry::VertexHandle{}));

    print_result("select vertex invalido", result);
    print_mesh_selection(editor);

    expect(stats, !result.success, "select vertex invalido falhou");
    expect(stats, editor.selection().mesh().empty(), "handle invalido nao alterou selecao");
    expect(stats, history.empty(), "handle invalido nao entrou no historico");

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleMeshComponentSelectionCommand>(geometry::EdgeHandle{}));

    print_result("toggle edge invalido", result);
    print_mesh_selection(editor);

    expect(stats, !result.success, "toggle edge invalido falhou");
    expect(stats, editor.selection().mesh().empty(), "toggle invalido nao alterou selecao");
    expect(stats, history.empty(), "toggle invalido nao entrou no historico");

    std::cout << "\n=== Select vertex deve selecionar somente v0 ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SelectMeshComponentCommand>(v0));

    print_result("select vertex v0", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "select vertex v0 executou com sucesso");
    expect(stats, editor.selection().mesh().vertices().size() == 1u, "ha 1 vertex selecionado");
    expect(stats, editor.selection().mesh().vertices().contains(v0), "v0 esta selecionado");
    expect(stats, editor.selection().mesh().edges().empty(), "edges continuam vazias");
    expect(stats, editor.selection().mesh().loops().empty(), "loops continuam vazios");
    expect(stats, editor.selection().mesh().faces().empty(), "faces continuam vazias");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Vertex, "granularidade virou Vertex");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "scope ficou ActiveMesh");
    expect(stats, history.undo_size() == 1u, "select vertex entrou no historico");

    std::cout << "\n=== Select vertex v1 deve substituir v0 ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SelectMeshComponentCommand>(v1));

    print_result("select vertex v1", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "select vertex v1 executou com sucesso");
    expect(stats, editor.selection().mesh().vertices().size() == 1u, "continua havendo 1 vertex selecionado");
    expect(stats, !editor.selection().mesh().vertices().contains(v0), "v0 foi removido");
    expect(stats, editor.selection().mesh().vertices().contains(v1), "v1 esta selecionado");
    expect(stats, history.undo_size() == 2u, "select vertex v1 entrou no historico");

    std::cout << "\n=== Undo deve restaurar v0 ===\n";

    result = history.undo(dispatcher);

    print_result("undo select vertex v1", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "undo select vertex v1 executou com sucesso");
    expect(stats, editor.selection().mesh().vertices().size() == 1u, "undo restaurou 1 vertex");
    expect(stats, editor.selection().mesh().vertices().contains(v0), "undo restaurou v0");
    expect(stats, !editor.selection().mesh().vertices().contains(v1), "undo removeu v1");
    expect(stats, history.undo_size() == 1u, "undo size voltou para 1");
    expect(stats, history.redo_size() == 1u, "redo size virou 1");

    std::cout << "\n=== Redo deve reaplicar v1 ===\n";

    result = history.redo(dispatcher);

    print_result("redo select vertex v1", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "redo select vertex v1 executou com sucesso");
    expect(stats, editor.selection().mesh().vertices().size() == 1u, "redo manteve 1 vertex");
    expect(stats, editor.selection().mesh().vertices().contains(v1), "redo reaplicou v1");
    expect(stats, !editor.selection().mesh().vertices().contains(v0), "redo removeu v0 novamente");
    expect(stats, history.undo_size() == 2u, "undo size voltou para 2");
    expect(stats, history.redo_size() == 0u, "redo size voltou para 0");

    std::cout << "\n=== Toggle vertex v2 deve adicionar v2 sem remover v1 ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleMeshComponentSelectionCommand>(v2));

    print_result("toggle vertex v2", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "toggle vertex v2 executou com sucesso");
    expect(stats, editor.selection().mesh().vertices().size() == 2u, "ha 2 vertices selecionados");
    expect(stats, editor.selection().mesh().vertices().contains(v1), "v1 continua selecionado");
    expect(stats, editor.selection().mesh().vertices().contains(v2), "v2 foi adicionado");
    expect(stats, history.undo_size() == 3u, "toggle v2 entrou no historico");

    std::cout << "\n=== Toggle vertex v1 deve remover v1 e manter v2 ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleMeshComponentSelectionCommand>(v1));

    print_result("toggle vertex v1", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "toggle vertex v1 executou com sucesso");
    expect(stats, editor.selection().mesh().vertices().size() == 1u, "ha 1 vertex selecionado apos remocao");
    expect(stats, !editor.selection().mesh().vertices().contains(v1), "v1 foi removido");
    expect(stats, editor.selection().mesh().vertices().contains(v2), "v2 continua selecionado");
    expect(stats, history.undo_size() == 4u, "toggle remove v1 entrou no historico");

    std::cout << "\n=== Undo do toggle v1 deve restaurar v1 + v2 ===\n";

    result = history.undo(dispatcher);

    print_result("undo toggle vertex v1", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "undo toggle vertex v1 executou com sucesso");
    expect(stats, editor.selection().mesh().vertices().size() == 2u, "undo restaurou 2 vertices");
    expect(stats, editor.selection().mesh().vertices().contains(v1), "undo restaurou v1");
    expect(stats, editor.selection().mesh().vertices().contains(v2), "undo manteve v2");
    expect(stats, history.undo_size() == 3u, "undo size voltou para 3");
    expect(stats, history.redo_size() == 1u, "redo size virou 1");

    std::cout << "\n=== Select edge deve limpar vertices e selecionar e0 ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SelectMeshComponentCommand>(e0));

    print_result("select edge e0", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "select edge e0 executou com sucesso");
    expect(stats, editor.selection().mesh().vertices().empty(), "select edge limpou vertices");
    expect(stats, editor.selection().mesh().edges().size() == 1u, "ha 1 edge selecionada");
    expect(stats, editor.selection().mesh().edges().contains(e0), "e0 esta selecionada");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Edge, "granularidade virou Edge");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "scope ficou ActiveMesh");
    expect(stats, history.redo_size() == 0u, "novo comando limpou pilha de redo");

    std::cout << "\n=== Toggle edge e1 deve adicionar e1 ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleMeshComponentSelectionCommand>(e1));

    print_result("toggle edge e1", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "toggle edge e1 executou com sucesso");
    expect(stats, editor.selection().mesh().edges().size() == 2u, "ha 2 edges selecionadas");
    expect(stats, editor.selection().mesh().edges().contains(e0), "e0 continua selecionada");
    expect(stats, editor.selection().mesh().edges().contains(e1), "e1 foi adicionada");

    std::cout << "\n=== Select loop deve limpar edges e selecionar l0 ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SelectMeshComponentCommand>(l0));

    print_result("select loop l0", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "select loop l0 executou com sucesso");
    expect(stats, editor.selection().mesh().edges().empty(), "select loop limpou edges");
    expect(stats, editor.selection().mesh().loops().size() == 1u, "ha 1 loop selecionado");
    expect(stats, editor.selection().mesh().loops().contains(l0), "l0 esta selecionado");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Loop, "granularidade virou Loop");

    std::cout << "\n=== Select face deve limpar loops e selecionar f0 ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<SelectMeshComponentCommand>(f0));

    print_result("select face f0", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "select face f0 executou com sucesso");
    expect(stats, editor.selection().mesh().loops().empty(), "select face limpou loops");
    expect(stats, editor.selection().mesh().faces().size() == 1u, "ha 1 face selecionada");
    expect(stats, editor.selection().mesh().faces().contains(f0), "f0 esta selecionada");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Face, "granularidade virou Face");

    std::cout << "\n=== Toggle face f0 deve remover face ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<ToggleMeshComponentSelectionCommand>(f0));

    print_result("toggle face f0", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "toggle face f0 executou com sucesso");
    expect(stats, editor.selection().mesh().faces().empty(), "f0 foi removida");
    expect(stats, editor.selection().mesh().empty(), "selecao de componentes ficou vazia");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Face, "granularidade continuou Face");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "scope continuou ActiveMesh");

    std::cout << "\n=== Undo do toggle face deve restaurar f0 ===\n";

    result = history.undo(dispatcher);

    print_result("undo toggle face f0", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "undo toggle face f0 executou com sucesso");
    expect(stats, editor.selection().mesh().faces().size() == 1u, "undo restaurou 1 face");
    expect(stats, editor.selection().mesh().faces().contains(f0), "undo restaurou f0");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Face, "undo manteve/restaurou Face");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "undo manteve/restaurou ActiveMesh");

    std::cout << "\n=== ClearMeshSelectionCommand deve limpar selecao final ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<ClearMeshSelectionCommand>());

    print_result("clear mesh selection", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "clear mesh selection executou com sucesso");
    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "clear preservou active mesh");
    expect(stats, editor.selection().mesh().empty(), "clear limpou componentes");

    std::cout << "\n=== Undo do clear deve restaurar f0 ===\n";

    result = history.undo(dispatcher);

    print_result("undo clear mesh selection", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "undo clear mesh selection executou com sucesso");
    expect(stats, editor.selection().mesh().faces().size() == 1u, "undo do clear restaurou face");
    expect(stats, editor.selection().mesh().faces().contains(f0), "undo do clear restaurou f0");
    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "undo do clear restaurou active mesh");

    std::cout << "\n=== Resultado final ===\n";
    std::cout << "passed: " << stats.passed << '\n';
    std::cout << "failed: " << stats.failed << '\n';

    if (stats.failed == 0) {
        std::cout << "\n[OK] Mesh component selection commands smoke test passou.\n";
        return 0;
    }

    std::cout << "\n[FAIL] Mesh component selection commands smoke test encontrou problemas.\n";
    return 1;
}