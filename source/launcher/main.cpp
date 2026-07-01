/* *
 * SPDX-FileCopyrightText: 2026 Icaro2M
 * SPDX-License-Identifier: Apache-2.0
 */

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/CommandResult.h"
#include "editor/command/selection/ClearMeshSelectionCommand.h"
#include "editor/command/selection/MeshSelectionSnapshot.h"
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
        std::cout << "  empty: " << (mesh.empty() ? "true" : "false") << '\n';
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor ClearMeshSelectionCommand Smoke Test ===\n\n";

    TestStats stats{};

    Editor editor{};
    CommandDispatcher dispatcher(editor);
    HistoryStack history{};

    const SceneNodeId meshNode = editor.scene().create_mesh("Editable Mesh");

    const geometry::VertexHandle v0{ 1 };
    const geometry::VertexHandle v1{ 2 };
    const geometry::EdgeHandle e0{ 3 };
    const geometry::LoopHandle l0{ 4 };
    const geometry::FaceHandle f0{ 5 };

    expect(stats, meshNode.is_valid(), "mesh node criado com id valido");
    expect(stats, editor.scene().find_mesh(meshNode) != nullptr, "mesh node pode ser encontrado como MeshNode");
    expect(stats, editor.selection().mesh().empty(), "selecao de mesh comeca vazia");
    expect(stats, history.empty(), "historico comeca vazio");

    std::cout << "\n=== Preparando selecao de componentes ===\n";

    editor.selection().mesh().set_active_mesh(meshNode);
    editor.selection().mesh().add_vertex(v0);
    editor.selection().mesh().add_vertex(v1);
    editor.selection().mesh().add_edge(e0);
    editor.selection().mesh().add_loop(l0);
    editor.selection().mesh().add_face(f0);
    editor.selection().mesh().set_hovered_vertex(v1);
    editor.selection().set_granularity(SelectionGranularity::Face);
    editor.selection().set_scope(SelectionScope::ActiveMesh);

    print_mesh_selection(editor);

    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "active mesh foi definido");
    expect(stats, editor.selection().mesh().vertices().size() == 2u, "2 vertices selecionados");
    expect(stats, editor.selection().mesh().edges().size() == 1u, "1 edge selecionada");
    expect(stats, editor.selection().mesh().loops().size() == 1u, "1 loop selecionado");
    expect(stats, editor.selection().mesh().faces().size() == 1u, "1 face selecionada");
    expect(stats, editor.selection().mesh().hovered_vertex() == v1, "hovered vertex foi definido");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Face, "granularidade preparada como Face");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "scope preparado como ActiveMesh");

    std::cout << "\n=== Teste direto do MeshSelectionSnapshot ===\n";

    MeshSelectionSnapshot snapshot{};
    snapshot.capture(editor.selection());

    editor.selection().mesh().add_vertex(geometry::VertexHandle{ 99 });
    editor.selection().mesh().add_edge(geometry::EdgeHandle{ 100 });
    editor.selection().mesh().set_hovered_face(f0);
    editor.selection().set_granularity(SelectionGranularity::Vertex);

    snapshot.restore(editor.selection());

    print_mesh_selection(editor);

    expect(stats, snapshot.is_valid(), "snapshot ficou valido apos capture");
    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "snapshot restaurou active mesh");
    expect(stats, editor.selection().mesh().vertices().size() == 2u, "snapshot restaurou somente os 2 vertices originais");
    expect(stats, editor.selection().mesh().edges().size() == 1u, "snapshot restaurou somente a edge original");
    expect(stats, editor.selection().mesh().loops().size() == 1u, "snapshot restaurou o loop original");
    expect(stats, editor.selection().mesh().faces().size() == 1u, "snapshot restaurou a face original");
    expect(stats, editor.selection().mesh().vertices().contains(v0), "snapshot restaurou v0");
    expect(stats, editor.selection().mesh().vertices().contains(v1), "snapshot restaurou v1");
    expect(stats, !editor.selection().mesh().vertices().contains(geometry::VertexHandle{ 99 }), "snapshot removeu vertex extra");
    expect(stats, !editor.selection().mesh().edges().contains(geometry::EdgeHandle{ 100 }), "snapshot removeu edge extra");
    expect(stats, editor.selection().mesh().hovered_vertex() == v1, "snapshot restaurou hovered vertex");
    expect(stats, editor.selection().mesh().hovered_face().is_invalid(), "snapshot restaurou hovered face invalido");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Face, "snapshot restaurou granularidade Face");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "snapshot restaurou scope ActiveMesh");

    std::cout << "\n=== ClearMeshSelectionCommand deve limpar componentes e preservar active mesh ===\n";

    const std::size_t undoBeforeClear = history.undo_size();

    CommandResult result = history.execute(
        dispatcher,
        std::make_unique<ClearMeshSelectionCommand>());

    print_result("clear mesh selection", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "clear mesh selection executou com sucesso");
    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "clear preservou active mesh");
    expect(stats, editor.selection().mesh().empty(), "clear removeu todos os componentes selecionados");
    expect(stats, editor.selection().mesh().hovered_vertex().is_invalid(), "clear removeu hovered vertex");
    expect(stats, mesh_selection_count(editor) == 0u, "contagem total de componentes virou zero");
    expect(stats, history.undo_size() == undoBeforeClear + 1u, "clear entrou no historico");
    expect(stats, history.can_undo(), "historico permite undo");

    std::cout << "\n=== Undo deve restaurar selecao anterior ===\n";

    result = history.undo(dispatcher);

    print_result("undo clear mesh selection", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "undo do clear executou com sucesso");
    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "undo restaurou active mesh");
    expect(stats, editor.selection().mesh().vertices().size() == 2u, "undo restaurou vertices");
    expect(stats, editor.selection().mesh().edges().size() == 1u, "undo restaurou edge");
    expect(stats, editor.selection().mesh().loops().size() == 1u, "undo restaurou loop");
    expect(stats, editor.selection().mesh().faces().size() == 1u, "undo restaurou face");
    expect(stats, editor.selection().mesh().vertices().contains(v0), "undo restaurou v0");
    expect(stats, editor.selection().mesh().vertices().contains(v1), "undo restaurou v1");
    expect(stats, editor.selection().mesh().edges().contains(e0), "undo restaurou e0");
    expect(stats, editor.selection().mesh().loops().contains(l0), "undo restaurou l0");
    expect(stats, editor.selection().mesh().faces().contains(f0), "undo restaurou f0");
    expect(stats, editor.selection().mesh().hovered_vertex() == v1, "undo restaurou hovered vertex");
    expect(stats, editor.selection().granularity() == SelectionGranularity::Face, "undo restaurou granularidade Face");
    expect(stats, editor.selection().scope() == SelectionScope::ActiveMesh, "undo restaurou scope ActiveMesh");
    expect(stats, history.redo_size() == 1u, "historico possui 1 redo");

    std::cout << "\n=== Redo deve limpar novamente ===\n";

    result = history.redo(dispatcher);

    print_result("redo clear mesh selection", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "redo do clear executou com sucesso");
    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "redo preservou active mesh");
    expect(stats, editor.selection().mesh().empty(), "redo limpou componentes novamente");
    expect(stats, mesh_selection_count(editor) == 0u, "redo deixou contagem total zero");
    expect(stats, history.undo_size() == undoBeforeClear + 1u, "redo devolveu comando para undo");
    expect(stats, history.redo_size() == 0u, "redo consumiu pilha de redo");

    std::cout << "\n=== Clear em selecao ja vazia tambem deve ser undoable ===\n";

    result = history.execute(
        dispatcher,
        std::make_unique<ClearMeshSelectionCommand>());

    print_result("clear mesh selection vazio", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "clear vazio executou com sucesso");
    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "clear vazio preservou active mesh");
    expect(stats, editor.selection().mesh().empty(), "clear vazio manteve componentes vazios");
    expect(stats, history.undo_size() == undoBeforeClear + 2u, "clear vazio entrou no historico");

    result = history.undo(dispatcher);

    print_result("undo clear vazio", result);
    print_mesh_selection(editor);

    expect(stats, result.success, "undo do clear vazio executou com sucesso");
    expect(stats, editor.selection().mesh().active_mesh() == meshNode, "undo clear vazio restaurou active mesh");
    expect(stats, editor.selection().mesh().empty(), "undo clear vazio manteve selecao vazia");

    std::cout << "\n=== Resultado final ===\n";
    std::cout << "passed: " << stats.passed << '\n';
    std::cout << "failed: " << stats.failed << '\n';

    if (stats.failed == 0) {
        std::cout << "\n[OK] ClearMeshSelectionCommand smoke test passou.\n";
        return 0;
    }

    std::cout << "\n[FAIL] ClearMeshSelectionCommand smoke test encontrou problemas.\n";
    return 1;
}