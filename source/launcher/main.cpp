// SPDX-FileCopyrightText: 2026 Icaro2M
// SPDX-License-Identifier: Apache-2.0

#include "editor/Editor.h"
#include "editor/command/CommandDispatcher.h"
#include "editor/command/document/ClearSceneCommand.h"
#include "editor/command/document/ImportMeshCommand.h"
#include "editor/history/HistoryStack.h"
#include "editor/scene/MeshNode.h"
#include "editor/scene/SceneNode.h"
#include "kernel/geometry/mesh/LEM.h"
#include "kernel/geometry/mesh/LEMEditor.h"
#include "kernel/geometry/mesh/LEMHandles.h"

#include <glm/vec3.hpp>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

    using namespace locus::editor;
    using namespace locus::kernel::geometry;

    int g_failures = 0;

    void check(bool condition, const std::string& message)
    {
        if (condition) {
            std::cout << "[OK] " << message << '\n';
            return;
        }

        std::cout << "[FAIL] " << message << '\n';
        ++g_failures;
    }

    void print_result(const std::string& label, const CommandResult& result)
    {
        std::cout << label << '\n';
        std::cout << "  success: " << (result.success ? "true" : "false") << '\n';
        std::cout << "  message: " << result.message << '\n';
    }

    void print_scene_counts(const std::string& label, const Editor& editor)
    {
        std::cout << label << '\n';
        std::cout << "  nodes: " << editor.scene().tree().size() << '\n';
        std::cout << "  roots: " << editor.scene().tree().roots().size() << '\n';
        std::cout << "  object selection: " << editor.selection().objects().size() << '\n';
        std::cout << "  active object valid: "
            << (editor.selection().objects().active().is_valid() ? "true" : "false") << '\n';
        std::cout << "  active mesh valid: "
            << (editor.selection().mesh().active_mesh().is_valid() ? "true" : "false") << '\n';
    }

    void print_mesh_counts(const std::string& label, const LEM& mesh)
    {
        std::cout << label << '\n';
        std::cout << "  vertices: " << mesh.vertex_count() << '\n';
        std::cout << "  edges: " << mesh.edge_count() << '\n';
        std::cout << "  loops: " << mesh.loop_count() << '\n';
        std::cout << "  faces: " << mesh.face_count() << '\n';
    }

    LEM make_triangle_mesh()
    {
        LEM mesh{};
        LEMEditor editor(mesh);

        const VertexHandle v0 = editor.add_vertex(glm::vec3{ 0.0f, 0.0f, 0.0f });
        const VertexHandle v1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, 0.0f });
        const VertexHandle v2 = editor.add_vertex(glm::vec3{ 0.0f, 1.0f, 0.0f });

        editor.add_face(std::vector<VertexHandle>{ v0, v1, v2 });

        return mesh;
    }

    LEM make_quad_mesh()
    {
        LEM mesh{};
        LEMEditor editor(mesh);

        const VertexHandle v0 = editor.add_vertex(glm::vec3{ -1.0f, 0.0f, -1.0f });
        const VertexHandle v1 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f, -1.0f });
        const VertexHandle v2 = editor.add_vertex(glm::vec3{ 1.0f, 0.0f,  1.0f });
        const VertexHandle v3 = editor.add_vertex(glm::vec3{ -1.0f, 0.0f,  1.0f });

        editor.add_face(std::vector<VertexHandle>{ v0, v1, v2, v3 });

        return mesh;
    }

    const MeshNode* find_mesh(const Editor& editor, SceneNodeId id)
    {
        return editor.scene().find_mesh(id);
    }

    MeshNode* find_mesh(Editor& editor, SceneNodeId id)
    {
        return editor.scene().find_mesh(id);
    }

} // namespace

int main()
{
    std::cout << "=== Locus3D Editor Document Commands Smoke Test ===\n\n";

    Editor editor;
    CommandDispatcher dispatcher(editor);
    HistoryStack history;

    check(editor.scene().tree().empty(), "cena comeca vazia");
    check(history.empty(), "historico comeca vazio");

    std::cout << "\n=== ImportMeshCommand ===\n";

    SceneNodeId importedNode{};

    {
        auto command = std::make_unique<ImportMeshCommand>(
            make_quad_mesh(),
            "Imported Quad",
            SceneNodeId{},
            true);

        ImportMeshCommand* commandPtr = command.get();

        const CommandResult result = history.execute(dispatcher, std::move(command));
        print_result("import mesh", result);

        check(result.success, "import mesh executou com sucesso");
        check(history.undo_size() == 1u, "import mesh entrou no historico");
        check(history.redo_size() == 0u, "redo continua vazio apos import");

        if (result.success) {
            importedNode = commandPtr->imported_node();
        }

        check(importedNode.is_valid(), "id importado e valido");
        check(editor.scene().tree().size() == 1u, "cena tem 1 node apos import");
        check(editor.scene().tree().roots().size() == 1u, "node importado e root");

        const MeshNode* node = find_mesh(editor, importedNode);
        check(node != nullptr, "node importado pode ser encontrado como MeshNode");

        if (node) {
            check(node->metadata().name == "Imported Quad", "nome do mesh importado foi aplicado");
            print_mesh_counts("malha importada", node->mesh());
            check(node->mesh().vertex_count() == 4u, "malha importada tem 4 vertices");
            check(node->mesh().edge_count() == 4u, "malha importada tem 4 edges");
            check(node->mesh().loop_count() == 4u, "malha importada tem 4 loops");
            check(node->mesh().face_count() == 1u, "malha importada tem 1 face");
        }

        check(editor.selection().objects().contains(importedNode), "node importado foi selecionado");
        check(editor.selection().objects().active() == importedNode, "node importado virou objeto ativo");
        check(editor.selection().mesh().active_mesh() == importedNode, "node importado virou active mesh");
        check(editor.selection().granularity() == SelectionGranularity::Object, "granularidade ficou Object");
        check(editor.selection().scope() == SelectionScope::Scene, "scope ficou Scene");

        print_scene_counts("estado apos import", editor);
    }

    std::cout << "\n=== Undo/Redo ImportMeshCommand ===\n";

    {
        const CommandResult undoResult = history.undo(dispatcher);
        print_result("undo import mesh", undoResult);

        check(undoResult.success, "undo import executou com sucesso");
        check(history.undo_size() == 0u, "undo removeu import da pilha de undo");
        check(history.redo_size() == 1u, "undo colocou import na pilha de redo");
        check(editor.scene().tree().empty(), "undo import removeu node da cena");
        check(!editor.selection().objects().contains(importedNode), "undo import removeu objeto da selecao");
        check(editor.selection().objects().active().is_invalid(), "undo import limpou objeto ativo");
        check(editor.selection().mesh().active_mesh().is_invalid(), "undo import limpou active mesh");

        print_scene_counts("estado apos undo import", editor);

        const CommandResult redoResult = history.redo(dispatcher);
        print_result("redo import mesh", redoResult);

        check(redoResult.success, "redo import executou com sucesso");
        check(history.undo_size() == 1u, "redo devolveu import para pilha de undo");
        check(history.redo_size() == 0u, "redo limpou pilha de redo");
        check(editor.scene().tree().size() == 1u, "redo import restaurou 1 node");
        check(editor.scene().find_node(importedNode) != nullptr, "redo import restaurou mesmo id");
        check(editor.selection().objects().contains(importedNode), "redo import restaurou selecao do importado");
        check(editor.selection().objects().active() == importedNode, "redo import restaurou objeto ativo");
        check(editor.selection().mesh().active_mesh() == importedNode, "redo import restaurou active mesh");

        const MeshNode* node = find_mesh(editor, importedNode);
        check(node != nullptr, "redo import restaurou MeshNode");

        if (node) {
            print_mesh_counts("malha apos redo import", node->mesh());
            check(node->mesh().vertex_count() == 4u, "redo import restaurou 4 vertices");
            check(node->mesh().edge_count() == 4u, "redo import restaurou 4 edges");
            check(node->mesh().face_count() == 1u, "redo import restaurou 1 face");
        }

        print_scene_counts("estado apos redo import", editor);
    }

    std::cout << "\n=== ImportMeshCommand com parent ===\n";

    SceneNodeId parentNode{};
    SceneNodeId childMesh{};

    {
        parentNode = editor.scene().create_empty("Parent Empty");
        check(parentNode.is_valid(), "parent empty criado com id valido");

        auto command = std::make_unique<ImportMeshCommand>(
            make_triangle_mesh(),
            "Child Triangle",
            parentNode,
            true);

        ImportMeshCommand* commandPtr = command.get();

        const CommandResult result = history.execute(dispatcher, std::move(command));
        print_result("import mesh com parent", result);

        check(result.success, "import com parent executou com sucesso");
        check(history.undo_size() == 2u, "import com parent entrou no historico");

        if (result.success) {
            childMesh = commandPtr->imported_node();
        }

        check(childMesh.is_valid(), "child mesh tem id valido");

        const SceneNode* parent = editor.scene().find_node(parentNode);
        const SceneNode* child = editor.scene().find_node(childMesh);
        const MeshNode* childAsMesh = find_mesh(editor, childMesh);

        check(parent != nullptr, "parent existe apos import com parent");
        check(child != nullptr, "child existe apos import com parent");
        check(childAsMesh != nullptr, "child pode ser encontrado como MeshNode");

        if (parent && child) {
            check(child->parent() == parentNode, "child aponta para parent correto");
            check(parent->children().size() == 1u, "parent possui 1 filho");
            check(parent->children().front() == childMesh, "filho do parent e o mesh importado");
        }

        if (childAsMesh) {
            print_mesh_counts("malha filha importada", childAsMesh->mesh());
            check(childAsMesh->mesh().vertex_count() == 3u, "malha filha tem 3 vertices");
            check(childAsMesh->mesh().edge_count() == 3u, "malha filha tem 3 edges");
            check(childAsMesh->mesh().face_count() == 1u, "malha filha tem 1 face");
        }

        check(editor.selection().objects().active() == childMesh, "child mesh virou objeto ativo");
        check(editor.selection().mesh().active_mesh() == childMesh, "child mesh virou active mesh");

        print_scene_counts("estado apos import com parent", editor);
    }

    std::cout << "\n=== Undo/Redo ImportMeshCommand com parent ===\n";

    {
        const CommandResult undoResult = history.undo(dispatcher);
        print_result("undo import com parent", undoResult);

        check(undoResult.success, "undo import com parent executou com sucesso");
        check(editor.scene().find_node(parentNode) != nullptr, "undo import com parent manteve parent");
        check(editor.scene().find_node(childMesh) == nullptr, "undo import com parent removeu child");
        check(editor.selection().objects().active() != childMesh, "undo import com parent limpou active child");
        check(editor.selection().mesh().active_mesh() != childMesh, "undo import com parent limpou active mesh child");

        const SceneNode* parent = editor.scene().find_node(parentNode);
        if (parent) {
            check(parent->children().empty(), "parent ficou sem filhos apos undo");
        }

        const CommandResult redoResult = history.redo(dispatcher);
        print_result("redo import com parent", redoResult);

        check(redoResult.success, "redo import com parent executou com sucesso");
        check(editor.scene().find_node(parentNode) != nullptr, "redo import com parent manteve parent");
        check(editor.scene().find_node(childMesh) != nullptr, "redo import com parent restaurou child");

        parent = editor.scene().find_node(parentNode);
        const SceneNode* child = editor.scene().find_node(childMesh);

        if (parent && child) {
            check(child->parent() == parentNode, "redo restaurou parent do child");
            check(parent->children().size() == 1u, "redo restaurou filho no parent");
            check(parent->children().front() == childMesh, "redo restaurou child correto no parent");
        }

        check(editor.selection().objects().active() == childMesh, "redo import com parent restaurou active child");
        check(editor.selection().mesh().active_mesh() == childMesh, "redo import com parent restaurou active mesh child");

        print_scene_counts("estado apos redo import com parent", editor);
    }

    std::cout << "\n=== ClearSceneCommand ===\n";

    {
        check(editor.scene().tree().size() == 3u, "antes do clear cena tem 3 nodes");
        check(editor.scene().find_node(importedNode) != nullptr, "mesh root existe antes do clear");
        check(editor.scene().find_node(parentNode) != nullptr, "parent existe antes do clear");
        check(editor.scene().find_node(childMesh) != nullptr, "child existe antes do clear");

        auto command = std::make_unique<ClearSceneCommand>();

        const CommandResult result = history.execute(dispatcher, std::move(command));
        print_result("clear scene", result);

        check(result.success, "clear scene executou com sucesso");
        check(history.undo_size() == 3u, "clear scene entrou no historico");
        check(history.redo_size() == 0u, "redo ficou vazio apos clear");
        check(editor.scene().tree().empty(), "clear scene deixou cena vazia");
        check(editor.selection().objects().empty(), "clear scene limpou selecao de objetos");
        check(editor.selection().mesh().active_mesh().is_invalid(), "clear scene limpou active mesh");
        check(editor.selection().mesh().empty(), "clear scene limpou selecao de componentes");

        print_scene_counts("estado apos clear", editor);
    }

    std::cout << "\n=== Undo/Redo ClearSceneCommand ===\n";

    {
        const CommandResult undoResult = history.undo(dispatcher);
        print_result("undo clear scene", undoResult);

        check(undoResult.success, "undo clear executou com sucesso");
        check(history.undo_size() == 2u, "undo clear removeu clear da pilha de undo");
        check(history.redo_size() == 1u, "undo clear colocou clear na pilha de redo");
        check(editor.scene().tree().size() == 3u, "undo clear restaurou 3 nodes");
        check(editor.scene().find_node(importedNode) != nullptr, "undo clear restaurou mesh root");
        check(editor.scene().find_node(parentNode) != nullptr, "undo clear restaurou parent");
        check(editor.scene().find_node(childMesh) != nullptr, "undo clear restaurou child");

        const MeshNode* rootMesh = find_mesh(editor, importedNode);
        const MeshNode* restoredChild = find_mesh(editor, childMesh);
        const SceneNode* restoredParent = editor.scene().find_node(parentNode);
        const SceneNode* child = editor.scene().find_node(childMesh);

        check(rootMesh != nullptr, "undo clear restaurou root como MeshNode");
        check(restoredChild != nullptr, "undo clear restaurou child como MeshNode");
        check(restoredParent != nullptr, "undo clear restaurou parent como SceneNode");

        if (rootMesh) {
            print_mesh_counts("root mesh apos undo clear", rootMesh->mesh());
            check(rootMesh->mesh().vertex_count() == 4u, "undo clear restaurou root com 4 vertices");
            check(rootMesh->mesh().face_count() == 1u, "undo clear restaurou root com 1 face");
        }

        if (restoredChild) {
            print_mesh_counts("child mesh apos undo clear", restoredChild->mesh());
            check(restoredChild->mesh().vertex_count() == 3u, "undo clear restaurou child com 3 vertices");
            check(restoredChild->mesh().face_count() == 1u, "undo clear restaurou child com 1 face");
        }

        if (restoredParent && child) {
            check(child->parent() == parentNode, "undo clear restaurou parent do child");
            check(restoredParent->children().size() == 1u, "undo clear restaurou lista de filhos do parent");
            check(restoredParent->children().front() == childMesh, "undo clear restaurou child correto");
        }

        check(editor.selection().objects().active() == childMesh, "undo clear restaurou objeto ativo anterior");
        check(editor.selection().mesh().active_mesh() == childMesh, "undo clear restaurou active mesh anterior");
        check(editor.selection().objects().contains(childMesh), "undo clear restaurou selecao do child");

        print_scene_counts("estado apos undo clear", editor);

        const CommandResult redoResult = history.redo(dispatcher);
        print_result("redo clear scene", redoResult);

        check(redoResult.success, "redo clear executou com sucesso");
        check(history.undo_size() == 3u, "redo clear devolveu clear para undo");
        check(history.redo_size() == 0u, "redo clear limpou redo");
        check(editor.scene().tree().empty(), "redo clear deixou cena vazia novamente");
        check(editor.selection().objects().empty(), "redo clear limpou selecao novamente");
        check(editor.selection().mesh().active_mesh().is_invalid(), "redo clear limpou active mesh novamente");

        print_scene_counts("estado apos redo clear", editor);
    }

    std::cout << "\n=== ClearSceneCommand em cena vazia ===\n";

    {
        const std::size_t undoBefore = history.undo_size();

        auto command = std::make_unique<ClearSceneCommand>();

        const CommandResult result = history.execute(dispatcher, std::move(command));
        print_result("clear scene vazia", result);

        check(!result.success, "clear scene em cena vazia falhou");
        check(history.undo_size() == undoBefore, "clear scene vazio nao entrou no historico");
        check(editor.scene().tree().empty(), "cena continuou vazia");
    }

    std::cout << "\n=== Resultado Final ===\n";

    if (g_failures == 0) {
        std::cout << "Todos os testes passaram.\n";
        return 0;
    }

    std::cout << g_failures << " teste(s) falharam.\n";
    return 1;
}